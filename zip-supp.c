/*
  $NiH$

  zip-supp.c -- support code for zip files
  Copyright (C) 1999 Dieter Baron and Thomas Klaunser

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
		zip->zf->zn, zip->zf->entry[idx].fn,
		zip_err_str[zip_err]);
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
			zip->zf->zn, zip->zf->entry[idx].fn,
			zip_err_str[zip_err]);
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
			zip->zf->zn, zip->zf->entry[idx].fn,
			zip_err_str[zip_err]);
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
		    zip? (zip->zf? (zip->zf->zn? zip->zf->zn
				    : "(null)")
			  :"(null)")
		    :"(null)", zip_err_str[zip_err]);
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
    int i;

    z->nrom = 0;
    z->rom = NULL;
    z->zf = NULL;

    if ((zf=zip_open(z->name, 0))==NULL) {
	/* no error if file doesn't exist */
	if (zip_err != ZERR_NOENT)
	    fprintf(stderr, "%s: error opening '%s': %s\n", prg,
		    z->name, zip_err_str[zip_err]);
	return -1;
    }

    z->rom = (struct rom *)xmalloc(sizeof(struct rom)*(zf->nentry));
    z->nrom = zf->nentry;
    z->zf = zf;

    for (i=0; i<zf->nentry; i++) {
	z->rom[i].name = xstrdup(zf->entry[i].fn);
	z->rom[i].size = zf->entry[i].meta->uncomp_size;
	z->rom[i].crc = zf->entry[i].meta->crc;
	z->rom[i].state = ROM_0;
    }	

    return i;
}
