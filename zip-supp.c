/*
  $NiH: zip-supp.c,v 1.19 2003/03/16 10:21:36 wiz Exp $

  zip-supp.c -- support code for zip files
  Copyright (C) 1999 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <errno.h>

#include "types.h"
#include "error.h"
#include "dbl.h"
#include "funcs.h"
#include "romutil.h"
#include "xmalloc.h"

#define MAXFNLEN 1024
#define BUFSIZE 8192

extern char *prg;

int
findcrc(struct zfile *zip, int idx, int romsize, unsigned long wcrc)
{
    struct zip_file *zff;
    unsigned long crc;
    char buf[BUFSIZE];
    int n, left, offset;

    if ((zff = zip_fopen_index(zip->zf, idx)) == NULL) {
	fprintf(stderr, "%s: %s: can't open file '%s': %s\n", prg,
		zip->name, zip_get_name(zip->zf, idx),
		zip_strerror(zip->zf));
	return -1;
    }

    offset = 0;
    while (offset+romsize <= zip->rom[idx].size) {
	left = BUFSIZE;
	crc = crc32(0, NULL, 0);
	n = romsize;
	while (n > 0) {
	    if (left > n)
		left = n;
	    if (zip_fread(zff, buf, left) != left) {
		fprintf(stderr, "%s: %s: %s: read error: %s\n", prg,
			zip->name, zip_get_name(zip->zf, idx),
			zip_strerror(zip->zf));
		zip_fclose(zff);
		return -1;
	    }
	    crc = crc32(crc, buf, left);
	    n -= left;
	}

	if (crc == wcrc)
	    break;

	offset += romsize;
    }

    if (zip_fclose(zff)) {
	fprintf(stderr, "%s: %s: %s: close error: %s\n", prg,
			zip->name, zip_get_name(zip->zf, idx),
			zip_strerror(zip->zf));
	return -1;
    }
    
    if (crc == wcrc)
	return offset;
	    
    return -1;
}




int
zfile_free(struct zfile *zip)
{
    int i, ret;

    if (zip == NULL)
	return 0;
    
    free(zip->name);
    for (i=0; i<zip->nrom; i++)
	free(zip->rom[i].name);

    if (zip->nrom)
	free(zip->rom);

    if (zip->zf) {
	if ((ret=zip_close(zip->zf))) {
	    /* error closing, so zip is still valid */
	    fprintf(stderr, "%s: %s: close error: %s\n", prg,
		    zip? (zip->zf? (zip->name ? zip->name
				    : "(null)")
			  :"(null)")
		    :"(null)", zip_strerror(zip->zf));
	    /* discard all changes and close zipfile */
	    zip_unchange_all(zip->zf);
	    zip_close(zip->zf);
	}
    }
    
    free(zip);

    return ret;
}



int
readinfosfromzip (struct zfile *z)
{
    struct zip *zf;
    struct zip_stat zsb;
    int i;
    /* number of valid entries found in zipfile */
    int count;
    int zerr = 0;

    z->nrom = 0;
    z->rom = NULL;
    z->zf = NULL;

    if ((zf=zip_open(z->name, 0, &zerr))==NULL) {
	/* no error if file doesn't exist */
	if (zip_err != ZERR_NOENT) {
	    char errstr[1024];

	    (void)zip_error_str(errstr, sizeof(errstr),
				zerr, errno);
	    fprintf(stderr, "%s: error opening '%s': %s\n", prg,
		    z->name, errstr);
	}

	return -1;
    }

    z->nrom = zip_get_num_files(zf);
    if (z->nrom < 0) {
	(void)zip_close(zf);
	return -1;
    }
    z->rom = (struct rom *)xmalloc(sizeof(struct rom)*z->nrom);
    z->zf = zf;

    count = 0;
    for (i=0; i<z->nrom; i++) {
	if (zip_stat_index(zf, i, &zsb) == -1) {
	    fprintf(stderr, "%s: error stat()ing index %d in `%s': %s\n",
		    prg, i, z->name, zip_strerror(zf));
	    continue;
	}

	z->rom[count].name = xstrdup(zip_get_name(zf, i));
	z->rom[count].size = zsb.size;
	z->rom[count].crc = zsb.crc;
	z->rom[count].state = ROM_0;
	count++;
    }	

    return count;
}
