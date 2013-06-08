/*
  archive.c -- information about an archive
  Copyright (C) 1999-2013 Dieter Baron and Thomas Klausner

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "memdb.h"
#include "util.h"
#include "xmalloc.h"

#define BUFSIZE 8192

static int get_hashes(struct zip_file *, off_t, hashes_t *);
static int read_infos_from_zip(archive_t *);

int _archive_global_flags = 0;



archive_t *
archive_by_id(int id)
{
    archive_t *a;

    a = (archive_t *)memdb_get_ptr_by_id(id);

    if (a != NULL)
	a->refcount++;

    return a;
}



int
archive_close_zip(archive_t *a)
{
    if (archive_zip(a) == NULL)
	return 0;

#ifdef FD_DEBUGGING
    fprintf(stderr, "zip_close %s\n", archive_name(a));
#endif
    if (zip_close(archive_zip(a)) < 0) {
	/* error closing, so zip is still valid */
	myerror(ERRZIP, "error closing zip: %s", zip_strerror(archive_zip(a)));

	/* XXX: really do this here? */
	/* discard all changes and close zipfile */
	zip_unchange_all(archive_zip(a));
	zip_close(archive_zip(a));
	archive_zip(a) = NULL;
	return -1;
    }

    archive_zip(a) = NULL;

    return 0;
}



int
archive_ensure_zip(archive_t *a)
{
    int flags;
    
    if (archive_zip(a))
	return 0;

    if (archive_filetype(a) != TYPE_ROM)
	return -1;

    flags = (a->flags & ARCHIVE_FL_CHECK_INTEGRITY) ? ZIP_CHECKCONS : 0;
    if (a->flags & ARCHIVE_FL_CREATE)
	flags |= ZIP_CREATE;

    if ((archive_zip(a)=my_zip_open(archive_name(a), flags)) == NULL)
	return -1;
#ifdef FD_DEBUGGING
    fprintf(stderr, "zip_open: %s\n", archive_name(a));
#endif

    return 0;
}



int
archive_file_compute_hashes(archive_t *a, int idx, int hashtypes)
{
    hashes_t h;
    struct zip *za;
    struct zip_file *zf;
    file_t *r;

    r = archive_file(a, idx);

    if ((hashes_types(file_hashes(r)) & hashtypes) == hashtypes)
	return 0;

    if (archive_ensure_zip(a) != 0)
	return -1;

    za = archive_zip(a);

    if ((zf=zip_fopen_index(za, idx, 0)) == NULL) {
	myerror(ERRDEF, "%s: %s: cannot open: %s",
		archive_name(a), file_name(r), zip_strerror(za));
	file_status(r) = STATUS_BADDUMP;
	return -1;
    }

    hashes_types(&h) = hashtypes;
    if (get_hashes(zf, file_size(r), &h) < 0) {
	myerror(ERRDEF, "%s: %s: read error: %s",
		archive_name(a), file_name(r), zip_strerror(za));
	zip_fclose(zf);
	file_status(r) = STATUS_BADDUMP;
	return -1;
    }

    zip_fclose(zf);

    if (hashtypes & HASHES_TYPE_CRC) {
	if (file_hashes(r)->crc != h.crc) {
	    myerror(ERRDEF, "%s: %s: CRC error: %lx != %lx",
		    archive_name(a), file_name(r),
		    h.crc, file_hashes(r)->crc);
	    file_status(r) = STATUS_BADDUMP;
	    return -1;
	}
    }
    hashes_copy(file_hashes(r), &h);

    return 0;
}



off_t
archive_file_find_offset(archive_t *a, int idx, int size, const hashes_t *h)
{
    struct zip_file *zf;
    hashes_t hn;
    int found;
    off_t offset;

    hashes_init(&hn);
    hashes_types(&hn) = hashes_types(h);

    if (archive_ensure_zip(a) < 0)
	return -1;

    seterrinfo(zip_get_name(archive_zip(a), idx, 0), archive_name(a));
    if ((zf = zip_fopen_index(archive_zip(a), idx, 0)) == NULL) {
	myerror(ERRZIPFILE, "can't open file: %s",
		zip_strerror(archive_zip(a)));
	return -1;
    }

    found = 0;
    offset = 0;
    while ((uint64_t)offset+size <= file_size(archive_file(a, idx))) {
	if (get_hashes(zf, size, &hn) < 0) {
	    myerror(ERRZIPFILE, "read error: %s",
		    zip_strerror(archive_zip(a)));
	    zip_fclose(zf);
	    return -1;
	}
	
	if (hashes_cmp(h, &hn) == HASHES_CMP_MATCH) {
	    found = 1;
	    break;
	}

	offset += size;
    }

    if (zip_fclose(zf)) {
	myerror(ERRZIPFILE, "close error: %s", zip_strerror(archive_zip(a)));
	return -1;
    }
    
    if (found)
	return offset;
	    
    return -1;
}



int
archive_file_index_by_name(const archive_t *a, const char *name)
{
    int i;

    for (i=0; i<archive_num_files(a); i++) {
	if (strcmp(file_name(archive_file(a, i)), name) == 0) {
	    if (file_where(archive_file(a, i)) == FILE_DELETED)
		return -1;
	    return i;
	}
    }

    return -1;
}



int
archive_free(archive_t *a)
{
    int ret;

    if (a == NULL)
	return 0;

    if (--a->refcount != 0)
	return 0;

    if (a->flags & ARCHIVE_IFL_MODIFIED) {
	/* XXX: warn about freeing modified archive */
    }

    ret = archive_close_zip(a);

    if (a->flags & ARCHIVE_FL_NOCACHE)
	archive_real_free(a);

    return ret;
}



void
archive_global_flags(int fl, bool setp)
{
    if (setp)
	_archive_global_flags |= fl;
    else
	_archive_global_flags &= ~fl;
}



archive_t *
archive_new(const char *name, filetype_t ft, where_t where, int flags)
{
    archive_t *a;
    int i, id;

    if ((a=memdb_get_ptr(name)) != 0) {
	/* XXX: check for compatibility of a->flags and flags */
	a->refcount++;
	return a;
    }

    a = xmalloc(sizeof(*a));
    a->id = -1;
    a->name = xstrdup(name);
    a->refcount = 1;
    a->where = where;
    a->files = array_new(sizeof(file_t));
    a->za = NULL;
    a->flags = ((flags|_archive_global_flags) & ARCHIVE_FL_MASK);

    switch (ft) {
    case TYPE_ROM:
    case TYPE_SAMPLE:
	archive_filetype(a) = TYPE_ROM;
	if (read_infos_from_zip(a) < 0) {
	    if (!(a->flags & ARCHIVE_FL_CREATE)) {
		archive_real_free(a);
		return NULL;
	    }
	}
	break;

    case TYPE_DISK:
	archive_filetype(a) = TYPE_DISK;
	/* XXX */
	break;

    default:
	archive_real_free(a);
	return NULL;
    }

    for (i=0; i<archive_num_files(a); i++) {
	/* XXX: file_state(archive_file(a, i)) = FILE_UNKNOWN; */
	file_where(archive_file(a, i)) = FILE_INZIP;
    }

    if (!(a->flags & ARCHIVE_FL_NOCACHE)) {
	if ((id=memdb_put_ptr(name, a)) < 0) {
	    archive_real_free(a);
	    return NULL;
	}
	a->id = id;

	if (IS_EXTERNAL(archive_where(a)))
	    memdb_file_insert_archive(a);
    }

    return a;
}



void
archive_real_free(archive_t *a)
{
    if (a == NULL)
	return;

    archive_close_zip(a);
    free(a->name);
    array_free(archive_files(a), file_finalize);
    free(a);
}



int
archive_refresh(archive_t *a)
{
    /* XXX: handle TYPE_DISK */

    archive_close_zip(a);
    array_truncate(archive_files(a), 0, file_finalize);

    read_infos_from_zip(a);

    return 0;
}



bool
archive_is_empty(const archive_t *a)
{
    int i;

    for (i=0; i<archive_num_files(a); i++)
	if (file_where(archive_file(a, i)) != FILE_DELETED)
	    return false;

    return true;
}



static int
get_hashes(struct zip_file *zf, off_t len, struct hashes *h)
{
    hashes_update_t *hu;
    unsigned char buf[BUFSIZE];
    off_t n;

    hu = hashes_update_new(h);

    while (len > 0) {
	n = len > sizeof(buf) ? sizeof(buf) : len;

	if (zip_fread(zf, buf, n) != n)
	    return -1;

	hashes_update(hu, buf, n);
	len -= n;
    }

    hashes_update_final(hu);

    return 0;
}



static int
match_detector(struct zip *za, int idx, file_t *r)
{
    struct zip_file *zf;
    int ret;

    if ((zf=zip_fopen_index(za, idx, 0)) == NULL) {
	myerror(ERRZIP, "error opening index %d: %s", idx, zip_strerror(za));
	file_status(r) = STATUS_BADDUMP;
	return -1;
    }

    ret = detector_execute(detector, r, (detector_read_cb)zip_fread, zf);

    zip_fclose(zf);

    return ret;
}



static int
read_infos_from_zip(archive_t *a)
{
    struct zip *za;
    struct zip_stat zsb;
    file_t *r;
    int i;

    if (archive_ensure_zip(a) < 0)
	return -1;

    za = archive_zip(a);

    seterrinfo(NULL, archive_name(a));

    for (i=0; i<zip_get_num_files(za); i++) {
	if (zip_stat_index(za, i, 0, &zsb) == -1) {
	    myerror(ERRZIP, "error stat()ing index %d: %s",
		    i, zip_strerror(za));
	    continue;
	}

	r = (file_t *)array_grow(archive_files(a), file_init);
	file_size(r) = zsb.size;
	file_name(r) = xstrdup(zsb.name);
	file_status(r) = STATUS_OK;

	hashes_init(file_hashes(r));
	file_hashes(r)->types = HASHES_TYPE_CRC;
	file_hashes(r)->crc = zsb.crc;

	if (detector)
	    match_detector(za, i, r);

	if (a->flags & ARCHIVE_FL_CHECK_INTEGRITY)
	    archive_file_compute_hashes(a, i, romhashtypes);
    }

    return 0;
}



bool
archive_is_torrentzipped(archive_t *a)
{
    if (archive_filetype(a) != TYPE_ROM)
	return true;

    if (a->za == NULL)
	return true;

    return zip_get_archive_flag(a->za, ZIP_AFL_TORRENT, ZIP_FL_UNCHANGED) == 1;
}
