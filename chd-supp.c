/*
  $NiH: chd-supp.c,v 1.3 2004/04/26 11:49:37 dillo Exp $

  chd-supp.c -- support code for chd files
  Copyright (C) 2004 Dieter Baron and Thomas Klausner

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



#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "romutil.h"

#define HEADERLEN	120
#define TAG		"MComprHD"
#define OFF_VERSION	12
#define OFF_MD5		44
#define OFF_SHA1	80

#define GET_LONG(b)	(((b)[0]<<24)|((b)[1]<<16)|((b)[2]<<8)|(b)[3])

extern char *prg;



/*
  fill in d->md5 and d->sha1 hashes from CHD file d->name
  returns 0 on success, -1 on error.
*/

int
readinfosfromchd(struct disk *d)
{
    FILE *f;
    unsigned char b[HEADERLEN];
    unsigned long version;

    hashes_init(&d->hashes);

    if ((f=fopen(d->name, "rb")) == NULL) {
	/* no error if file doesn't exist */
	if (errno != ENOENT) {
	    fprintf(stderr, "%s: error opening '%s': %s\n",
		    prg, d->name, strerror(errno));
	}
	return -1;
    }

    if (fread(b, sizeof(b), 1, f) != 1) {
	fprintf(stderr, "%s: error reading '%s': %s\n",
		prg, d->name, strerror(errno));
	fclose(f);
	return -1;
    }

    fclose(f);
    
    if (strncmp(b, TAG, 8) != 0) {
	fprintf(stderr, "%s: '%s' is not a chd file\n",
		prg, d->name);
	return -1;
    }

    version = GET_LONG(b+12);

    if (version > 3)
	fprintf(stderr, "%s: warning: chd file '%s' has unknown version %lu\n",
		prg, d->name, version);

    d->hashes.types |= GOT_MD5;
    memcpy(d->hashes.md5, b+OFF_MD5, sizeof(d->hashes.md5));
    if (version >= 3) {
	d->hashes.types |= GOT_SHA1;
	memcpy(d->hashes.sha1, b+OFF_SHA1, sizeof(d->hashes.sha1));
    }

    return 0;
}
