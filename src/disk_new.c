/*
  disk_new.c -- create / free disk structure from image
  Copyright (C) 2004-2007 Dieter Baron and Thomas Klausner

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

#include "chd.h"
#include "disk.h"
#include "error.h"
#include "globals.h"
#include "memdb.h"
#include "xmalloc.h"



static int get_hashes(struct chd *, struct hashes *);



disk_t *
disk_new(const char *name, int flags)
{
    disk_t *d;
    struct chd *chd;
    hashes_t *h;
    int err, id;

    if (name == NULL)
	return NULL;

    if ((d=memdb_get_ptr(name)) != 0) {
	d->refcount++;
	return d;
    }

    seterrinfo(name, NULL);
    if ((chd=chd_open(name, &err)) == NULL) {
	/* no error if file doesn't exist */
	if (!((err == CHD_ERR_OPEN && errno == ENOENT)
	      || ((flags & DISK_FL_QUIET) && err == CHD_ERR_NO_CHD))) {
	    /* XXX: include err */
	    myerror(ERRFILESTR, "error opening disk");
	}
	return NULL;
    }

    if (chd->flags & CHD_FLAG_HAS_PARENT) {
	chd_close(chd);
	myerror(ERRFILE, "error opening disk: parent image required");
	return NULL;
    }

    d = (disk_t *)xmalloc(sizeof(*d));
    disk_init(d);
    d->name = xstrdup(name);
    d->refcount = 1;

    h = disk_hashes(d);
    hashes_init(h);

    if (flags & DISK_FL_CHECK_INTEGRITY) {
	hashes_types(h) = diskhashtypes;
	
	err = get_hashes(chd, h);
	chd_close(chd);

	if (err < 0) {
	    disk_real_free(d);
	    return NULL;
	}

	if (diskhashtypes & HASHES_TYPE_MD5) {
	    if (!hashes_verify(h, HASHES_TYPE_MD5, chd->md5)) {
		myerror(ERRFILE, "md5 mismatch");
		disk_real_free(d);
		return NULL;
	    }
	}

	if (chd->version > 2 && (diskhashtypes & HASHES_TYPE_SHA1)) {
	    if (!hashes_verify(h, HASHES_TYPE_SHA1, chd->sha1)) {
		myerror(ERRFILE, "sha1 mismatch in '%s'");
		disk_real_free(d);
		return NULL;
	    }
	}		
    }
    else {
	chd_close(chd);
    
	hashes_set(h, HASHES_TYPE_MD5, chd->md5);
	if (chd->version > 2)
	    hashes_set(h, HASHES_TYPE_SHA1, chd->sha1);
    }

    if ((id=memdb_put_ptr(name, d)) < 0) {
	disk_real_free(d);
	return NULL;
    }
    d->id = id;

    return d;
}



void
disk_free(disk_t *d)
{
    if (d == NULL)
	return;

    if (--d->refcount != 0)
	return;

#ifdef DONT_CACHE_UNUSED
    if (_dmap)
	disk_map_delete(_dmap, disk_name(d));
    else
	disk_real_free(d);
#endif
}



void
disk_real_free(disk_t *d)
{
    if (d == NULL)
	return;

    disk_finalize(d);
    free(d);
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

	if (chd_read_hunk(chd, hunk, buf) != (int)chd->hunk_len) {
	    /* XXX: include chd->error */
	    myerror(ERRFILESTR, "error reading hunk %d", hunk);
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
