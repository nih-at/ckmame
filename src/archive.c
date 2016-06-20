/*
  archive.c -- information about an archive
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "archive.h"
#include "dbh_cache.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "memdb.h"
#include "util.h"
#include "xmalloc.h"

#define BUFSIZE 8192

static int archive_cache_is_up_to_date(archive_t *a);
static int cmp_file_by_name(const file_t *f, const char *name);
static int get_hashes(archive_t *, void *, off_t, struct hashes *);
static bool merge_files(archive_t *a, array_t *files);
static void replace_files(archive_t *a, array_t *files);


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
archive_check(archive_t *a)
{
    return a->ops->check(a);
}



int
archive_close(archive_t *a)
{
    int ret;
    seterrinfo(NULL, archive_name(a));
    
    ret = archive_commit(a);

    return a->ops->close(a) | ret;
}


int
archive_file_compute_hashes(archive_t *a, int idx, int hashtypes)
{
    hashes_t h;
    file_t *r;
    void *f;
    
    r = archive_file(a, idx);
    
    if ((hashes_types(file_hashes(r)) & hashtypes) == hashtypes)
	return 0;

    hashes_types(&h) = HASHES_TYPE_ALL;

    if ((f=a->ops->file_open(a, idx)) == NULL) {
	myerror(ERRDEF, "%s: %s: can't open: %s", archive_name(a), file_name(r), strerror(errno));
	file_status(r) = STATUS_BADDUMP;
	return -1;
    }
    
    if (get_hashes(a, f, file_size(r), &h) < 0) {
	myerror(ERRDEF, "%s: %s: can't compute hashes: %s", archive_name(a), file_name(r), strerror(errno));
	file_status(r) = STATUS_BADDUMP;
	a->ops->file_close(f);
	return -1;
    }

    a->ops->file_close(f);
    
    if (hashes_types(file_hashes(r)) & hashtypes & HASHES_TYPE_CRC) {
	if (file_hashes(r)->crc != h.crc) {
	    myerror(ERRDEF, "%s: %s: CRC error: %lx != %lx", archive_name(a), file_name(r), h.crc, file_hashes(r)->crc);
	    file_status(r) = STATUS_BADDUMP;
	    return -1;
	}
    }
    hashes_copy(file_hashes(r), &h);

    a->cache_changed = true;

    return 0;
}


off_t
archive_file_find_offset(archive_t *a, int idx, int size, const hashes_t *h)
{
    void *f;
    hashes_t hn;
    int found;
    off_t offset;
    
    hashes_init(&hn);
    hashes_types(&hn) = hashes_types(h);
    
    file_t *r = archive_file(a, idx);
    
    if ((f = a->ops->file_open(a, idx)) == NULL) {
	file_status(r) = STATUS_BADDUMP;
	return -1;
    }
    
    found = 0;
    offset = 0;
    while ((uint64_t)offset+size <= file_size(r)) {
	if (get_hashes(a, f, size, &hn) < 0) {
	    a->ops->file_close(f);
	    file_status(r) = STATUS_BADDUMP;
	    return -1;
	}
	
	if (hashes_cmp(h, &hn) == HASHES_CMP_MATCH) {
	    found = 1;
	    break;
	}
        
	offset += size;
    }

    a->ops->file_close(f);
    
    if (found)
	return offset;
    
    return -1;
}



int
archive_file_index_by_hashes(const archive_t *a, const hashes_t *h)
{
    int i;

    if (h->types == 0) {
	return -1;
    }
    
    for (i=0; i<archive_num_files(a); i++) {
	file_t *f = archive_file(a, i);
	
	if (hashes_cmp(h, file_hashes(f)) == HASHES_CMP_MATCH) {
	    if (file_where(f) == FILE_DELETED) {
		return -1;
	    }
	    return i;
	}
    }

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
archive_file_match_detector(archive_t *a, int idx)
{
    void *f;
    file_t *r;
    int ret;
    
    r = archive_file(a, idx);

    if ((f=a->ops->file_open(a, idx)) == NULL) {
	myerror(ERRZIP, "%s: can't open: %s", file_name(r), strerror(errno));
	file_status(r) = STATUS_BADDUMP;
	return -1;
    }

    ret = detector_execute(detector, r, a->ops->file_read, f);
    
    a->ops->file_close(f);
    
    return ret;
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
	/* TODO: warn about freeing modified archive */
    }

    ret = archive_close(a);

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


char *
archive_make_unique_name(archive_t *a, const char *name)
{
    char *unique, *p;
    char n[4];
    const char *ext;
    int idx, i, j;

    idx = archive_file_index_by_name(a, name);
    if (idx < 0)
	return xstrdup(name);

    unique = (char *)xmalloc(strlen(name)+5);

    ext = strrchr(name, '.');
    if (ext == NULL) {
	strcpy(unique, name);
	p = unique+strlen(unique);
	p[4] = '\0';
    }
    else {
	strncpy(unique, name, ext-name);
	p = unique + (ext-name);
	strcpy(p+4, ext);
    }	
    *(p++) = '-';

    for (i=0; i<1000; i++) {
	sprintf(n, "%03d", i);
	strncpy(p, n, 3);

	int exists = 0;
	for (j=0; j<archive_num_files(a); j++) {
	    if (j == idx)
		continue;
	    if (strcmp(file_name(archive_file(a, j)), unique) == 0) {
		exists = 1;
		break;
	    }
	}

	if (!exists)
	    return unique;
    }

    free(unique);
    return NULL;
}


archive_t *
archive_new(const char *name, filetype_t ft, where_t where, int flags)
{
    archive_t *a;
    int i, id;

    if ((a=memdb_get_ptr(name, ft)) != 0) {
	/* TODO: check for compatibility of a->flags and flags */
	a->refcount++;
	return a;
    }

    a = xmalloc(sizeof(*a));
    a->id = -1;
    a->name = xstrdup(name);
    a->refcount = 1;
    a->where = where;
    a->files = array_new(sizeof(file_t));
    a->ud = NULL;
    a->flags = ((flags|_archive_global_flags) & (ARCHIVE_FL_MASK|ARCHIVE_FL_HASHTYPES_MASK));
    a->mtime = 0;
    a->size = 0;

    switch (ft) {
    case TYPE_ROM:
    case TYPE_SAMPLE:
	archive_filetype(a) = TYPE_ROM;
	if ((roms_unzipped ? archive_init_dir(a) : archive_init_zip(a)) < 0) {
	    archive_real_free(a);
	    return NULL;
	}
        if (!archive_read_infos(a)) {
            /* TODO: return error if archive_read_infos failed for error other than archive doesn't exist */
            if (!(a->flags & ARCHIVE_FL_CREATE)) {
                archive_real_free(a);
                return NULL;
            }
	}
	break;

    default:
	archive_real_free(a);
	return NULL;
    }

    for (i=0; i<archive_num_files(a); i++) {
	/* TODO: file_state(archive_file(a, i)) = FILE_UNKNOWN; */
	file_where(archive_file(a, i)) = FILE_INZIP;
    }

    if (!(a->flags & ARCHIVE_FL_NOCACHE)) {
	if ((id=memdb_put_ptr(name, archive_filetype(a), a)) < 0) {
	    archive_real_free(a);
	    return NULL;
	}
	a->id = id;

	if (IS_EXTERNAL(archive_where(a)))
	    memdb_file_insert_archive(a);
    }

    return a;
}


archive_t *
archive_new_toplevel(const char *name, filetype_t ft, where_t where, int flags)
{
    char *slash;

    if (xasprintf(&slash, "%s/", name) < 0) {
        return NULL;
    }

    archive_t *a = archive_new(slash, ft, where, flags | ARCHIVE_FL_TOP_LEVEL_ONLY);

    free(slash);

    if (a != NULL && archive_num_files(a) == 0) {
        archive_close(a);
        a = NULL;
    }

    return a;
}


bool
archive_read_infos(archive_t *a)
{
    array_t *files_cache = NULL;

    a->cache_db = dbh_cache_get_db_for_archive(archive_name(a));
    a->cache_changed = false;
    if (a->cache_db) {
	a->cache_id = dbh_cache_get_archive_id(a->cache_db, archive_name(a));

	if (a->cache_id > 0) {
            files_cache = array_new(sizeof(file_t));
            if (!dbh_cache_read(a->cache_db, archive_name(a), files_cache)) {
                array_free(files_cache, file_finalize);
                return false;
            }

            switch (archive_cache_is_up_to_date(a)) {
                case -1:
                    array_free(files_cache, file_finalize);
                    return false;

                case 0:
                    break;

                case 1:
                    replace_files(a, files_cache);
                    return true;
            }

            if (a->ops->get_last_update) {
                a->cache_changed = true;
            }
        }
    }
    else {
	a->cache_id = 0;
    }

    if (!a->ops->read_infos(a)) {
	array_free(files_cache, file_finalize);
        a->cache_changed = true;
	return false;
    }

    if (!merge_files(a, files_cache)) {
        array_free(files_cache, file_finalize);
        return false;
    }

    if (files_cache) {
	array_free(files_cache, file_finalize);
	a->cache_changed = true;
    }
    else if (archive_num_files(a) > 0) {
        a->cache_changed = true;
    }

    return true;
}


void
archive_real_free(archive_t *a)
{
    if (a == NULL)
	return;

    archive_close(a);
    free(a->name);
    array_free(archive_files(a), file_finalize);
    free(a);
}


int
archive_refresh(archive_t *a)
{
    archive_close(a);
    array_truncate(archive_files(a), 0, file_finalize);
    archive_read_infos(a);

    return 0;
}


int
archive_register_cache_directory(const char *name)
{
    return dbh_cache_register_cache_directory(name);
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
archive_cache_is_up_to_date(archive_t *a)
{
    if (a->ops->get_last_update == NULL) {
        archive_mtime(a) = 0;
        archive_size(a) = 0;
        return 0;
    }

    time_t mtime_cache;
    off_t size_cache;

    if (!dbh_cache_get_archive_last_change(a->cache_db, a->cache_id, &mtime_cache, &size_cache)) {
        return -1;
    }

    if (!a->ops->get_last_update(a, &archive_mtime(a), &archive_size(a))) {
        return -1;
    }

    return (mtime_cache == archive_mtime(a) && size_cache == archive_size(a));
}


static int
cmp_file_by_name(const file_t *f, const char *name)
{
    return strcmp(file_name(f), name);
}


static int
get_hashes(archive_t *a, void *f, off_t len, struct hashes *h)
{
    hashes_update_t *hu;
    unsigned char buf[BUFSIZE];
    off_t n;
    
    hu = hashes_update_new(h);
    
    while (len > 0) {
	n = len > sizeof(buf) ? sizeof(buf) : len;
        
	if (a->ops->file_read(f, buf, n) != n)
	    return -1;
        
	hashes_update(hu, buf, n);
	len -= n;
    }
    
    hashes_update_final(hu);
    
    return 0;
}


static bool
merge_files(archive_t *a, array_t *files)
{
    int i, idx;

    for (i = 0; i < array_length(archive_files(a)); i++) {
	file_t *file = archive_file(a, i);

	if (files && (idx = array_find(files, file_name(file), cmp_file_by_name)) >= 0) {
	    file_t *cfile = array_get(files, idx);
	    if (file_mtime(file) == file_mtime(cfile) && file_compare_nsc(file, cfile)) {
		hashes_copy(file_hashes(file), file_hashes(cfile));
	    }
	}
	if (!hashes_has_type(file_hashes(file), HASHES_TYPE_CRC)) {
	    if (archive_file_compute_hashes(a, i, HASHES_TYPE_ALL) < 0) {
                file_status(file) = STATUS_BADDUMP;
                continue;
            }
	    if (detector) {
		archive_file_match_detector(a, i);
	    }
	}
    }

    return true;
}


static void
replace_files(archive_t *a, array_t *files)
{
    array_free(archive_files(a), file_finalize);
    archive_files(a) = files;
}


