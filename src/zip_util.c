/*
  zip_util.c -- utility functions for zip needed only by ckmame itself
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

#include <zip.h>

#include "error.h"
#include "funcs.h"
#include "xmalloc.h"


struct zip *
my_zip_open(const char *name, int flags) {
    struct zip *z;
    char errbuf[80];
    int err;

    z = zip_open(name, flags, &err);
    if (z == NULL) {
	zip_error_to_str(errbuf, sizeof(errbuf), err, errno);
	myerror(ERRDEF, "error %s zip archive '%s': %s", (flags & ZIP_CREATE ? "creating" : "opening"), name, errbuf);
    }

    return z;
}


int
my_zip_rename(struct zip *za, int idx, const char *name) {
    int zerr;
    zip_int64_t idx2;

    if (zip_rename(za, idx, name) == 0)
	return 0;

    zip_error_get(za, &zerr, NULL);

    if (zerr != ZIP_ER_EXISTS)
	return -1;

    idx2 = zip_name_locate(za, name, 0);
    if (idx2 == -1)
	return -1;
    if (my_zip_rename_to_unique(za, (zip_uint64_t)idx2) < 0)
	return -1;

    return zip_rename(za, idx, name);
}


int
my_zip_rename_to_unique(struct zip *za, zip_uint64_t idx) {
    char *unique, *p;
    char n[4];
    const char *name, *ext;
    int i, ret, zerr;

    if ((name = zip_get_name(za, idx, 0)) == NULL)
	return -1;

    unique = (char *)xmalloc(strlen(name) + 5);

    ext = strrchr(name, '.');
    if (ext == NULL) {
	strcpy(unique, name);
	p = unique + strlen(unique);
	p[4] = '\0';
    }
    else {
	strncpy(unique, name, ext - name);
	p = unique + (ext - name);
	strcpy(p + 4, ext);
    }
    *(p++) = '-';

    for (i = 0; i < 1000; i++) {
	sprintf(n, "%03d", i);
	strncpy(p, n, 3);

	ret = zip_rename(za, idx, unique);
	zip_error_get(za, &zerr, NULL);
	if (ret == 0 || zerr != ZIP_ER_EXISTS) {
	    free(unique);
	    return ret;
	}
    }

    free(unique);
    return -1;
}
