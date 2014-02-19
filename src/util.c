/*
  util.c -- utility functions
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "error.h"
#include "globals.h"
#include "util.h"
#include "xmalloc.h"


int
is_writable_directory(const char *name)
{
    struct stat st;

    if (stat(name, &st) < 0)
	return 0;

    if (!S_ISDIR(st.st_mode)) {
	errno = ENOTDIR;
	return 0;
    }

    return access(name, R_OK|W_OK|X_OK) == 0;
}


const char *
mybasename(const char *fname)
{
    const char *p;

    if ((p=strrchr(fname, '/')) == NULL)
	return fname;
    return p+1;
}



char *
mydirname(const char *fname)
{
    const char *p;
    char *d;
    size_t l;

    /* TODO: ignore trailing slashes */

    if ((p=strrchr(fname, '/')) == NULL)
	return xstrdup(".");

    l = p - fname;

    if (l == 0)
	return xstrdup("/");

    d = xmalloc(l+1);
    strncpy(d, fname, l);
    d[l] = '\0';
    return d;
}



char *
bin2hex(char *b, const unsigned char *s, unsigned int len)
{
    unsigned int i;

    for (i=0; i<len; i++)
	sprintf(b+2*i, "%02x", (unsigned char)s[i]);
    b[2*i] = '\0';

    return b;
}



#define HEX2BIN(c)	(((c)>='0' && (c)<='9') ? (c)-'0'	\
			 : ((c)>='A' && (c)<='F') ? (c)-'A'+10	\
			 : (c)-'a'+10)

int
hex2bin(unsigned char *t, const char *s, int unsigned tlen)
{
    unsigned int i;
    
    if (strspn(s, "0123456789AaBbCcDdEeFf") != tlen*2
	|| s[tlen*2] != '\0')
	return -1;

    for (i=0; i<tlen; i++)
	t[i] = HEX2BIN(s[i*2])<<4 | HEX2BIN(s[i*2+1]);
    
    return 0;
}



name_type_t
name_type(const char *name)
{
    size_t l;

    if (roms_unzipped) {
	struct stat st;

	if (stat(name, &st) < 0)
	    return NAME_UNKNOWN;
	if (S_ISDIR(st.st_mode))
	    return NAME_ZIP;
    }

    l = strlen(name);

    if (strchr(name, '.') == NULL)
	return NAME_NOEXT;

    if (l > 4) {
	if (strcmp(name+l-4, ".chd") == 0)
	    return NAME_CHD;
	if (!roms_unzipped && strcasecmp(name+l-4, ".zip") == 0)
	    return NAME_ZIP;
    }

    return NAME_UNKNOWN;
}

int
ensure_dir(const char *name, int strip_fname)
{
    const char *p;
    char *dir;
    struct stat st;
    int ret;
    
    if (strip_fname) {
	p = strrchr(name, '/');
	if (p == NULL)
	    dir = xstrdup(".");
	else {
	    dir = xmalloc(p-name+1);
	    strncpy(dir, name, p-name);
	    dir[p-name] = 0;
	}
	name = dir;
    }
    
    ret = 0;
    if (stat(name, &st) < 0) {
	if (strchr(name, '/')) {
	    ret = ensure_dir(name, 1);
	}
	if (ret == 0) {
	    if (mkdir(name, 0777) < 0) {
		myerror(ERRSTR, "mkdir '%s' failed", name);
		ret = -1;
	    }
	}
    }
    else if (!(st.st_mode & S_IFDIR)) {
	myerror(ERRDEF, "'%s' is not a directory", name);
	ret = -1;
    }
    
    if (strip_fname)
	free(dir);
    
    return ret;
}		    


const char *
get_directory(filetype_t ft)
{
    if (ft == TYPE_SAMPLE)
	return "samples";
    else if (rom_dir)
	return rom_dir;
    else
	return "roms";
}

int
remove_file_and_containing_empty_dirs(const char *name, const char *base)
{
    if (base == NULL) {
	errno = EINVAL;
	return -1;
    }

    size_t n = strlen(base);

    if (n >= strlen(name) || strncmp(base, name, n) != 0 || name[n] != '/') {
	errno = EINVAL;
	return -1;
    }

    if (unlink(name) < 0)
	return -1;

    if (strchr(name+n+1, '/') == NULL)
	return 0;

    char *tmp = xstrdup(name);
    char *r;
    while ((r=strrchr(tmp+n+1, '/')) != NULL) {
	*r = '\0';
	if (rmdir(tmp) < 0) {
	    free(tmp);
	    return errno == ENOTEMPTY ? 0 : -1;
	}
    }

    free(tmp);
    return 0;
}
