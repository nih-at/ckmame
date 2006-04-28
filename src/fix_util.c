/*
  $NiH: util2.c,v 1.11 2006/04/26 21:01:51 dillo Exp $

  util.c -- utility functions needed only by ckmame itself
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "xmalloc.h"



#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif



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
	if (mkdir(name, 0777) < 0) {
	    myerror(ERRSTR, "mkdir `%s' failed", name);
	    ret = -1;
	}
    }
    else if (!(st.st_mode & S_IFDIR)) {
	myerror(ERRDEF, "`%s' is not a directory", name);
	ret = -1;
    }

    if (strip_fname)
	free(dir);

    return ret;
}		    



char *
make_unique_name(const char *ext, const char *fmt, ...)
{
    char ret[MAXPATHLEN];
    int i, len;
    struct stat st;
    va_list ap;

    va_start(ap, fmt);
    len = vsnprintf(ret, sizeof(ret), fmt, ap);
    va_end(ap);

    /* already used space, "-XXX.", extension, 0 */
    if (len+5+strlen(ext)+1 > sizeof(ret)) {
	return NULL;
    }

    for (i=0; i<1000; i++) {
	sprintf(ret+len, "-%03d.%s", i, ext);

	if (stat(ret, &st) == -1 && errno == ENOENT)
	    return xstrdup(ret);
    }
    
    return NULL;
}



char *
make_needed_name(const rom_t *r)
{
    char crc[HASHES_SIZE_CRC*2+1];

    /* <needed_dir>/<crc>-nnn.zip */

    hash_to_string(crc, HASHES_TYPE_CRC, rom_hashes(r));

    return make_unique_name("zip", "%s/%s", needed_dir, crc);
}



char *
make_needed_name_disk(const disk_t *d)
{
    char md5[HASHES_SIZE_MD5*2+1];

    /* <needed_dir>/<md5>-nnn.zip */

    hash_to_string(md5, HASHES_TYPE_MD5, rom_hashes(d));

    return make_unique_name("chd", "%s/%s", needed_dir, md5);
}



int
rename_or_move(const char *old, const char *new)
{
    if (rename(old, new) < 0) {
	/* XXX: try move */
	seterrinfo(old, NULL);
	myerror(ERRFILESTR, "cannot rename to `%s'", new);
	return -1;
    }

    return 0;
}
