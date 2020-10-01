/*
  disk_new.c -- create / free disk structure from image
  Copyright (C) 2004-2014 Dieter Baron and Thomas Klausner

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


struct meta_hash {
    unsigned char tag[4];
    unsigned char sha1[HASHES_SIZE_SHA1];
};


static int get_hashes(struct chd *, struct hashes *);
static int meta_hash_cmp(const void *, const void *);


disk_t *
disk_new(const char *name, int flags) {
    disk_t *d;
    struct chd *chd;
    hashes_t *h;
    int err;
    int64_t id;

    if (name == NULL)
	return NULL;

    if ((d = memdb_get_ptr(name, TYPE_DISK)) != 0) {
	d->refcount++;
	return d;
    }

    seterrinfo(name, NULL);
    if ((chd = chd_open(name, &err)) == NULL) {
	/* no error if file doesn't exist */
	if (!((err == CHD_ERR_OPEN && errno == ENOENT) || ((flags & DISK_FL_QUIET) && err == CHD_ERR_NO_CHD))) {
	    /* TODO: include err */
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
	hashes_types(h) = romdb_hashtypes(db, TYPE_DISK);

	err = get_hashes(chd, h);

	if (err < 0) {
	    chd_close(chd);
	    disk_real_free(d);
	    return NULL;
	}

	if (romdb_hashtypes(db, TYPE_DISK) & HASHES_TYPE_MD5 & h->types) {
	    if (!hashes_verify(h, HASHES_TYPE_MD5, chd->md5)) {
		myerror(ERRFILE, "md5 mismatch");
		chd_close(chd);
		disk_real_free(d);
		return NULL;
	    }
	}

	if (chd->version > 2 && (romdb_hashtypes(db, TYPE_DISK) & HASHES_TYPE_SHA1 & h->types)) {
	    if (!hashes_verify(h, HASHES_TYPE_SHA1, chd->sha1)) {
		myerror(ERRFILE, "sha1 mismatch");
		chd_close(chd);
		disk_real_free(d);
		return NULL;
	    }
	}
    }
    else {
	h->types = 0;
    }

    if (chd->version < 4 && (hashes_types(h) & HASHES_TYPE_MD5) == 0)
	hashes_set(h, HASHES_TYPE_MD5, chd->md5);
    if (chd->version > 2 && (hashes_types(h) & HASHES_TYPE_SHA1) == 0)
	hashes_set(h, HASHES_TYPE_SHA1, chd->sha1);

    chd_close(chd);

    if ((id = memdb_put_ptr(name, TYPE_DISK, d)) < 0) {
	disk_real_free(d);
	return NULL;
    }
    d->id = id;

    return d;
}


void
disk_free(disk_t *d) {
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
disk_real_free(disk_t *d) {
    if (d == NULL)
	return;

    disk_finalize(d);
    free(d);
}


static int
get_hashes(struct chd *chd, struct hashes *h) {
    struct hashes h_raw;
    struct hashes_update *hu;
    uint32_t hunk;
    uint64_t n, len;
    unsigned char *buf;

    if (chd->version > 3) {
	/* version 4 only defines hash for SHA1 */
	h->types = HASHES_TYPE_SHA1;
    }

    h_raw.types = h->types;

    if (chd->version > 2)
	h_raw.types |= HASHES_TYPE_SHA1;
    if (chd->version < 4)
	h_raw.types |= HASHES_TYPE_MD5;

    hu = hashes_update_new(&h_raw);

    buf = xmalloc(chd->hunk_len);
    len = chd->total_len;
    for (hunk = 0; hunk < chd->total_hunks; hunk++) {
	n = chd->hunk_len > len ? len : chd->hunk_len;

	if (chd_read_hunk(chd, hunk, buf) != (int)chd->hunk_len) {
	    if (chd->error == CHD_ERR_NOTSUP) {
		myerror(ERRFILE, "warning: unsupported CHD type, integrity not checked");
		h->types = 0;
		hashes_update_discard(hu);
		return 0;
	    }
	    myerror(ERRFILESTR, "error reading hunk %d: error %d", hunk, chd->error);
	    free(buf);
	    hashes_update_final(hu);
	    return -1;
	}

	hashes_update(hu, buf, n);
	len -= n;
    }

    hashes_update_final(hu);

    if ((chd->version < 4 && memcmp(h_raw.md5, chd->md5, HASHES_SIZE_MD5) != 0) || (chd->version > 2 && memcmp(h_raw.sha1, chd->raw_sha1, HASHES_SIZE_SHA1) != 0)) {
	myerror(ERRFILE, "checksum mismatch for raw data");
	free(buf);
	return -1;
    }

    if (chd->version < 4) {
	hashes_copy(h, &h_raw);
    }
    else {
	struct hashes_update *hu_meta;
	struct chd_metadata_entry *meta, *e;
	struct meta_hash *meta_hash;
	int n_meta_hash, i;

	hu = hashes_update_new(h);
	hashes_update(hu, h_raw.sha1, HASHES_SIZE_SHA1);

	h_raw.types = HASHES_TYPE_SHA1;

	meta = chd_get_metadata_list(chd);

	n_meta_hash = 0;

	for (e = meta; e; e = e->next) {
	    if (e->flags & CHD_META_FL_CHECKSUM)
		n_meta_hash++;
	}

	meta_hash = xmalloc(n_meta_hash * sizeof(*meta_hash));

	len = chd->hunk_len; /* current size of buf */

	for (i = 0, e = meta; e; e = e->next) {
	    if ((e->flags & CHD_META_FL_CHECKSUM) == 0)
		continue;

	    if (e->length > len) {
		free(buf);
		len = e->length;
		buf = xmalloc(len);
	    }

	    if (chd_read_metadata(chd, e, buf) < 0) {
		/* TODO: include chd->error */
		myerror(ERRFILESTR, "error reading hunk %d", hunk);
		free(buf);
		free(meta_hash);
		hashes_update_final(hu);
		return -1;
	    }

	    hu_meta = hashes_update_new(&h_raw);
	    hashes_update(hu_meta, buf, e->length);
	    hashes_update_final(hu_meta);

	    memcpy(meta_hash[i].tag, e->tag, 4);
	    memcpy(meta_hash[i].sha1, h_raw.sha1, HASHES_SIZE_SHA1);
	    i++;
	}

	qsort(meta_hash, n_meta_hash, sizeof(*meta_hash), meta_hash_cmp);

	for (i = 0; i < n_meta_hash; i++) {
	    hashes_update(hu, meta_hash[i].tag, 4);
	    hashes_update(hu, meta_hash[i].sha1, HASHES_SIZE_SHA1);
	}
	hashes_update_final(hu);
	free(meta_hash);
    }

    free(buf);

    return 0;
}


static int
meta_hash_cmp(const void *a_, const void *b_) {
    const struct meta_hash *a, *b;
    int ret;

    a = a_;
    b = b_;

    ret = memcmp(a->tag, b->tag, sizeof(a->tag));
    if (ret == 0)
	ret = memcmp(a->sha1, b->sha1, sizeof(a->sha1));

    return ret;
}
