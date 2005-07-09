/*
  $NiH: chd-supp.c,v 1.3 2005/07/04 23:51:32 dillo Exp $

  chd-supp.c -- support code for chd files
  Copyright (C) 2004, 2005 Dieter Baron and Thomas Klausner

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "romutil.h"
#include "chd.h"
#include "chd-supp.h"
#include "xmalloc.h"

extern char *prg;

static int get_hashes(struct chd *, struct hashes *);



/*
  fill in d->md5 and d->sha1 hashes from CHD file d->name
  returns 0 on success, -1 on error.
*/

int
read_infos_from_chd(struct disk *d, int hashtypes)
{
    struct chd *chd;
    int err;

    hashes_init(&d->hashes);

    if ((chd=chd_open(d->name, &err)) == NULL) {
	/* no error if file doesn't exist */
	if (!(err == CHD_ERR_OPEN && errno == ENOENT)) {
	    /* XXX: include err */
	    fprintf(stderr, "%s: error opening '%s': %s\n",
		    prg, d->name, strerror(errno));
	}
	return -1;
    }

    if (hashtypes == 0) {
	d->hashes.types |= HASHES_TYPE_MD5;
	memcpy(d->hashes.md5, chd->md5, sizeof(d->hashes.md5));
	if (chd->version > 2) {
	    d->hashes.types |= HASHES_TYPE_SHA1;
	    memcpy(d->hashes.sha1, chd->sha1, sizeof(d->hashes.sha1));
	}
    }
    else {
	d->hashes.types = hashtypes;
	if (get_hashes(chd, &d->hashes) < 0) {
	    chd_close(chd);
	    return -1;
	}

	if (hashtypes & HASHES_TYPE_MD5) {
	    if (memcmp(d->hashes.md5, chd->md5, sizeof(d->hashes.md5)) != 0) {
		fprintf(stderr, "%s: md5 mismatch in '%s'\n",
		    prg, d->name);
		chd_close(chd);
		return -1;
	    }
	}
	else
	    memcpy(d->hashes.md5, chd->md5, sizeof(d->hashes.md5));

	if (chd->version > 2) {
	    if (hashtypes & HASHES_TYPE_SHA1) {
		if (memcmp(d->hashes.sha1, chd->sha1,
			   sizeof(d->hashes.sha1)) != 0) {
		    fprintf(stderr, "%s: sha1 mismatch in '%s'\n",
			    prg, d->name);
		    chd_close(chd);
		    return -1;
		}
	    }
	    else
		memcpy(d->hashes.sha1, chd->sha1, sizeof(d->hashes.sha1));
	}		
    }
    
    chd_close(chd);

    return 0;
}



static int
get_hashes(struct chd *chd, struct hashes *h)
{
    struct hashes_update *hu;
    unsigned int hunk, n;
    uint64_t len;
    unsigned char *buf;

    /* XXX: support CRC? */
    h->types &= ~HASHES_TYPE_CRC;

    hu = hashes_update_new(h);

    buf = xmalloc(chd->hunk_len);
    len = chd->total_len;
    for (hunk=0; hunk<chd->total_hunks; hunk++) {
	n = chd->hunk_len > len ? len : chd->hunk_len;

	if (chd_read_hunk(chd, hunk, buf) != chd->hunk_len) {
	    /* XXX: include chd->error */
	    fprintf(stderr, "%s: error reading hunk %d in '%s': %s\n",
		    prg, hunk, chd->name, strerror(errno));
	    free(buf);
	    return -1;
	}

	hashes_update(hu, buf, n);
	len -= n;
    }
    
    hashes_update_final(hu);
    free(buf);

    return 0;
}