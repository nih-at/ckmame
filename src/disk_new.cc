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
    unsigned char sha1[Hashes::SIZE_SHA1];
};


static int get_hashes(struct chd *, Hashes *);
static int meta_hash_cmp(const void *, const void *);


DiskPtr Disk::from_file(const std::string &name, int flags) {
    if (name.empty()) {
	return NULL;
    }

    auto disk = by_name(name);
    
    if (disk) {
        return disk;
    }

    seterrinfo(name, "");

    int err;
    auto chd = chd_open(name.c_str(), &err);
    if (chd == NULL) {
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

    disk = std::make_shared<Disk>();
    disk->name = name;

    if (flags & DISK_FL_CHECK_INTEGRITY) {
        disk->hashes.types = db->hashtypes(TYPE_DISK);

	err = get_hashes(chd, &disk->hashes);

	if (err < 0) {
	    chd_close(chd);
	    return NULL;
	}

        if (disk->hashes.has_type(Hashes::TYPE_MD5)) {
            if (!disk->hashes.verify(Hashes::TYPE_MD5, chd->md5)) {
		myerror(ERRFILE, "md5 mismatch");
		chd_close(chd);
		return NULL;
	    }
	}

        if (chd->version > 2 && disk->hashes.has_type(Hashes::TYPE_SHA1)) {
            if (!disk->hashes.verify(Hashes::TYPE_SHA1, chd->sha1)) {
		myerror(ERRFILE, "sha1 mismatch");
		chd_close(chd);
		return NULL;
	    }
	}
    }

    if (chd->version < 4 && !disk->hashes.has_type(Hashes::TYPE_MD5)) {
        disk->hashes.set(Hashes::TYPE_MD5, chd->md5);
    }
    if (chd->version > 2 && !disk->hashes.has_type(Hashes::TYPE_SHA1)) {
        disk->hashes.set(Hashes::TYPE_SHA1, chd->sha1);
    }

    chd_close(chd);

    disk->id = ++next_id;
    disk_by_id[disk->id] = disk;
    disk_by_name[name] = disk;

    return disk;
}


static int
get_hashes(struct chd *chd, Hashes *h) {
    Hashes h_raw;
    uint32_t hunk;
    uint64_t n, len;
    unsigned char *buf;

    if (chd->version > 3) {
	/* version 4 only defines hash for SHA1 */
	h->types = Hashes::TYPE_SHA1;
    }

    h_raw.types = h->types;

    if (chd->version > 2)
	h_raw.types |= Hashes::TYPE_SHA1;
    if (chd->version < 4)
	h_raw.types |= Hashes::TYPE_MD5;

    auto hu = Hashes::Update(&h_raw);

    buf = static_cast<unsigned char *>(xmalloc(chd->hunk_len));
    len = chd->total_len;
    for (hunk = 0; hunk < chd->total_hunks; hunk++) {
	n = chd->hunk_len > len ? len : chd->hunk_len;

	if (chd_read_hunk(chd, hunk, buf) != (int)chd->hunk_len) {
	    if (chd->error == CHD_ERR_NOTSUP) {
		myerror(ERRFILE, "warning: unsupported CHD type, integrity not checked");
		h->types = 0;
		return 0;
	    }
	    myerror(ERRFILESTR, "error reading hunk %d: error %d", hunk, chd->error);
	    free(buf);
	    return -1;
	}

	hu.update(buf, n);
	len -= n;
    }

    hu.end();

    if ((chd->version < 4 && memcmp(h_raw.md5, chd->md5, Hashes::SIZE_MD5) != 0) || (chd->version > 2 && memcmp(h_raw.sha1, chd->raw_sha1, Hashes::SIZE_SHA1) != 0)) {
	myerror(ERRFILE, "checksum mismatch for raw data");
	free(buf);
	return -1;
    }

    if (chd->version < 4) {
        *h = h_raw;
    }
    else {
	struct chd_metadata_entry *meta, *e;
	struct meta_hash *meta_hash;
	int n_meta_hash, i;

	hu = Hashes::Update(h);
        hu.update(h_raw.sha1, Hashes::SIZE_SHA1);

	h_raw.types = Hashes::TYPE_SHA1;

	meta = chd_get_metadata_list(chd);

	n_meta_hash = 0;

	for (e = meta; e; e = e->next) {
	    if (e->flags & CHD_META_FL_CHECKSUM)
		n_meta_hash++;
	}

	meta_hash = static_cast<struct meta_hash *>(xmalloc(n_meta_hash * sizeof(*meta_hash)));

	len = chd->hunk_len; /* current size of buf */

	for (i = 0, e = meta; e; e = e->next) {
	    if ((e->flags & CHD_META_FL_CHECKSUM) == 0)
		continue;

	    if (e->length > len) {
		free(buf);
		len = e->length;
		buf = static_cast<unsigned char *>(xmalloc(len));
	    }

	    if (chd_read_metadata(chd, e, buf) < 0) {
		/* TODO: include chd->error */
		myerror(ERRFILESTR, "error reading hunk %d", hunk);
		free(buf);
		free(meta_hash);
		return -1;
	    }

	    auto hu_meta = Hashes::Update(&h_raw);
            hu_meta.update(buf, e->length);
            hu_meta.end();

	    memcpy(meta_hash[i].tag, e->tag, 4);
	    memcpy(meta_hash[i].sha1, h_raw.sha1, Hashes::SIZE_SHA1);
	    i++;
	}

	qsort(meta_hash, n_meta_hash, sizeof(*meta_hash), meta_hash_cmp);

	for (i = 0; i < n_meta_hash; i++) {
            hu.update(meta_hash[i].tag, 4);
	    hu.update(meta_hash[i].sha1, Hashes::SIZE_SHA1);
	}
        hu.end();
	free(meta_hash);
    }

    free(buf);

    return 0;
}


static int
meta_hash_cmp(const void *a_, const void *b_) {
    const struct meta_hash *a, *b;
    int ret;

    a = static_cast<const struct meta_hash *>(a_);
    b = static_cast<const struct meta_hash *>(b_);

    ret = memcmp(a->tag, b->tag, sizeof(a->tag));
    if (ret == 0)
	ret = memcmp(a->sha1, b->sha1, sizeof(a->sha1));

    return ret;
}
