/*
  $NiH: zip-supp.c,v 1.1 2005/07/04 21:54:51 dillo Exp $

  zip-supp.c -- support code for zip files
  Copyright (C) 1999, 2004, 2005 Dieter Baron and Thomas Klausner

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



#include "config.h"

#include <errno.h>
#ifdef HAVE_MD5INIT
#include <md5.h>
#else
#include <md5_own.h>
#endif
#ifdef HAVE_SHA1INIT
#include <sha1.h>
#else
#include <sha1_own.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "types.h"
#include "error.h"
#include "dbl.h"
#include "funcs.h"
#include "romutil.h"
#include "xmalloc.h"

#define MAXFNLEN 1024
#define BUFSIZE 8192

extern char *prg;

static int get_hashes(struct zip_file *, off_t, struct hashes *);



int
findcrc(struct zfile *zip, int idx, int romsize, const struct hashes *h)
{
    struct zip_file *zf;
    struct hashes hn;
    unsigned int offset, found;

    hashes_init(&hn);
    hn.types = h->types;

    if ((zf = zip_fopen_index(zip->zf, idx, 0)) == NULL) {
	fprintf(stderr, "%s: %s: can't open file '%s': %s\n", prg,
		zip->name, zip_get_name(zip->zf, idx, 0),
		zip_strerror(zip->zf));
	return -1;
    }

    found = 0;
    offset = 0;
    while (offset+romsize <= zip->rom[idx].size) {
	if (get_hashes(zf, romsize, &hn) < 0) {
	    fprintf(stderr, "%s: %s: %s: read error: %s\n", prg,
		    zip->name, zip_get_name(zip->zf, idx, 0),
		    zip_strerror(zip->zf));
	    zip_fclose(zf);
	    return -1;
	}
	
	if (hashes_cmp(h, &hn) == HASHES_CMP_MATCH) {
	    found = 1;
	    break;
	}

	offset += romsize;
    }

    if (zip_fclose(zf)) {
	fprintf(stderr, "%s: %s: %s: close error: %s\n", prg,
			zip->name, zip_get_name(zip->zf, idx, 0),
			zip_strerror(zip->zf));
	return -1;
    }
    
    if (found)
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
read_infos_from_zip(struct zfile *z, int hashtypes)
{
    struct zip *za;
    struct zip_file *zf;
    struct zip_stat zsb;
    int i;
    /* number of valid entries found in zipfile */
    int count;
    int zerr = 0;

    z->nrom = 0;
    z->rom = NULL;
    z->zf = NULL;

    if ((za=zip_open(z->name, 0, &zerr))==NULL) {
	/* no error if file doesn't exist */
	if (zerr != ZIP_ER_OPEN && errno != ENOENT) {
	    char errstr[1024];

	    (void)zip_error_to_str(errstr, sizeof(errstr),
				   zerr, errno);
	    fprintf(stderr, "%s: error opening '%s': %s\n", prg,
		    z->name, errstr);
	}

	return -1;
    }

    z->nrom = zip_get_num_files(za);
    if (z->nrom < 0) {
	(void)zip_close(za);
	return -1;
    }
    z->rom = (struct rom *)xmalloc(sizeof(struct rom)*z->nrom);
    z->zf = za;

    count = 0;
    for (i=0; i<z->nrom; i++) {
	if (zip_stat_index(za, i, 0, &zsb) == -1) {
	    fprintf(stderr, "%s: error stat()ing index %d in `%s': %s\n",
		    prg, i, z->name, zip_strerror(za));
	    continue;
	}
	z->rom[count].size = zsb.size;

	hashes_init(&z->rom[count].hashes);
	if (hashtypes == 0) {
	    z->rom[count].hashes.types = HASHES_TYPE_CRC;
	    z->rom[count].hashes.crc = zsb.crc;
	}
	else {
	    if ((zf=zip_fopen_index(za, i, 0)) == NULL) {
		fprintf(stderr, "%s: error open()ing index %d in `%s': %s\n",
			prg, i, z->name, zip_strerror(za));
		continue;
	    }
	    z->rom[count].hashes.types = hashtypes;
	    get_hashes(zf, zsb.size, &z->rom[count].hashes);
	    zip_fclose(zf);
	    if (hashtypes & HASHES_TYPE_CRC) {
		if (z->rom[count].hashes.crc != zsb.crc) {
		    fprintf(stderr,
			    "%s: CRC error at index %d in `%s': %x != %lx\n",
			    prg, i, z->name, zsb.crc,
			    z->rom[count].hashes.crc);
		    continue;
		}
	    }
	    else {
		z->rom[count].hashes.types |= HASHES_TYPE_CRC;
		z->rom[count].hashes.crc = zsb.crc;
	    }
	}
	z->rom[count].name = xstrdup(zip_get_name(za, i, 0));
	z->rom[count].state = ROM_0;
	count++;
    }	

    return count;
}



static int
get_hashes(struct zip_file *zf, off_t len, struct hashes *h)
{
    unsigned long crc;
    MD5_CTX md5;
    SHA1_CTX sha1;
    unsigned char buf[BUFSIZE];
    int n;

    if (h->types & HASHES_TYPE_CRC)
	crc = crc32(0, NULL, 0);
    if (h->types & HASHES_TYPE_MD5)
	MD5Init(&md5);
    if (h->types & HASHES_TYPE_SHA1)
	SHA1Init(&sha1);

    while (len > 0) {
	n = len > sizeof(buf) ? sizeof(buf) : len;

	if (zip_fread(zf, buf, n) != n)
	    return -1;

	if (h->types & HASHES_TYPE_CRC)
	    crc = crc32(crc, (Bytef *)buf, n);
	if (h->types & HASHES_TYPE_MD5)
	    MD5Update(&md5, buf, n);
	if (h->types & HASHES_TYPE_SHA1)
	    SHA1Update(&sha1, buf, n);
	len -= n;
    }

    if (h->types & HASHES_TYPE_CRC)
	h->crc = crc;
    if (h->types & HASHES_TYPE_MD5)
	MD5Final(h->md5, &md5);
    if (h->types & HASHES_TYPE_SHA1)
	SHA1Final(h->sha1, &sha1);

    return 0;
}
