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
#include "ArchiveDir.h"
#include "ArchiveZip.h"
#include "dbh_cache.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "memdb.h"
#include "util.h"
#include "xmalloc.h"

#define BUFSIZE 8192

static int archive_cache_is_up_to_date(Archive *a);
static int cmp_file_by_name(const file_t *f, const char *name);
static int get_hashes(Archive *, void *, off_t, struct hashes *);
static bool merge_files(Archive *a, array_t *files);
static void replace_files(Archive *a, array_t *files);


int Archive::close() {
    int ret;
    seterrinfo(NULL, name.c_str());

    ret = commit();

    return close_xxx() | ret;
}

int Archive::file_compare_hashes(uint64_t index, const hashes_t *h) {
    auto rh = file_hashes(&files[index]);

    if ((hashes_types(rh) & hashes_types(h)) != hashes_types(h)) {
        file_compute_hashes(index, hashes_types(h) | romdb_hashtypes(db, TYPE_ROM));
    }

    if (file_status_(&files[index]) != STATUS_OK) {
        return HASHES_CMP_NOCOMMON;
    }

    return hashes_cmp(rh, h);
}


bool Archive::file_compute_hashes(uint64_t idx, int hashtypes) {
    hashes_t h;
    auto r = &files[idx];

    if ((hashes_types(file_hashes(r)) & hashtypes) == hashtypes)
	return true;

    hashes_types(&h) = HASHES_TYPE_ALL;

    auto f = file_open(idx);
    if (!f) {
	myerror(ERRDEF, "%s: %s: can't open: %s", name.c_str(), file_name(r), strerror(errno));
	file_status_(r) = STATUS_BADDUMP;
	return false;
    }

    if (!get_hashes(f.get(), file_size_(r), &h)) {
	myerror(ERRDEF, "%s: %s: can't compute hashes: %s", name.c_str(), file_name(r), strerror(errno));
	file_status_(r) = STATUS_BADDUMP;
	return false;
    }

    if (hashes_types(file_hashes(r)) & hashtypes & HASHES_TYPE_CRC) {
	if (file_hashes(r)->crc != h.crc) {
	    myerror(ERRDEF, "%s: %s: CRC error: %x != %x", name.c_str(), file_name(r), h.crc, file_hashes(r)->crc);
	    file_status_(r) = STATUS_BADDUMP;
	    return false;
	}
    }
    hashes_copy(file_hashes(r), &h);

    cache_changed = true;

    return true;
}


std::optional<size_t> Archive::file_find_offset(size_t idx, size_t size, const hashes_t *h) {
    hashes_t hn;

    hashes_init(&hn);
    hashes_types(&hn) = hashes_types(h);

    auto r = &files[idx];

    auto f = file_open(idx);
    if (!f) {
	file_status_(r) = STATUS_BADDUMP;
        return {};
    }

    auto found = false;
    size_t offset = 0;
    while (offset + size <= file_size_(r)) {
	if (!get_hashes(f.get(), size, &hn)) {
	    file_status_(r) = STATUS_BADDUMP;
            return {};
	}

	if (hashes_cmp(h, &hn) == HASHES_CMP_MATCH) {
	    found = true;
	    break;
	}

	offset += size;
    }

    if (found) {
	return offset;
    }

    return {};
}


std::optional<size_t> Archive::file_index_by_hashes(const hashes_t *h) const {
    if (h->types == 0) {
        return {};
    }

    for (size_t i = 0; i < files.size(); i++) {
        auto *f = &files[i];

	if (hashes_cmp(h, file_hashes(f)) == HASHES_CMP_MATCH) {
	    if (file_where(f) == FILE_DELETED) {
                return {};
	    }
	    return i;
	}
    }

    return {};
}


std::optional<size_t> Archive::file_index_by_name(const std::string &filename) const {
    for (size_t i = 0; i < files.size(); i++) {
        auto *f = &files[i];

        if (filename == file_name(f)) {
            if (file_where(f) == FILE_DELETED) {
                return {};
            }
	    return i;
	}
    }

    return {};
}


void Archive::file_match_detector(uint64_t index) {
    auto file = &files[index];

    auto fp = file_open(index);
    if (!fp) {
        myerror(ERRZIP, "%s: can't open: %s", file_name(file), strerror(errno));
        file_status_(file) = STATUS_BADDUMP;
    }
    
    detector_execute(detector, file, Archive::file_read_c, fp.get());
}


int64_t Archive::file_read_c(void *fp, void *data, uint64_t length) {
    auto file = static_cast<Archive::File *>(fp);
    
    return file->read(data, length);
}


void
archive_global_flags(int fl, bool setp) {
    if (setp)
	_archive_global_flags |= fl;
    else
	_archive_global_flags &= ~fl;
}


std::string Archive::make_unique_name(const std::string &filename) {
    char *unique, *p;
    char n[4];

    if (!file_index_by_name(filename).has_value()) {
        return filename;
    }

    unique = static_cast<char *>(xmalloc(filename.size() + 5));

    auto ext_index = filename.find_last_of(".");
    if (ext_index == std::string::npos) {
        strcpy(unique, name.c_str());
	p = unique + strlen(unique);
	p[4] = '\0';
    }
    else {
        strncpy(unique, name.c_str(), ext_index);
        p = unique + ext_index;
	strcpy(p + 4, name.c_str() + ext_index);
    }
    *(p++) = '-';

    for (int i = 0; i < 1000; i++) {
	sprintf(n, "%03d", i);
	strncpy(p, n, 3);

	auto exists = false;
        
        for (auto &file : files) {
            if (file.name == unique) {
                exists = true;
                break;
            }
	}

        if (!exists) {
	    return unique;
        }
    }

    free(unique);
    return "";
}

ArchivePtr Archive::open(const std::string &name, filetype_t filetype, where_t where, int flags) {
    auto it = archive_by_name.find(name);
    if (it != archive_by_name.end()) {
        return it->second;
    }

    ArchivePtr archive;
    
    try {
        switch (filetype) {
            case TYPE_ROM:
                if (roms_unzipped) {
                    archive = std::make_shared<ArchiveDir>(name, filetype, where, flags);
                }
                else {
                    archive = std::make_shared<ArchiveZip>(name, filetype, where, flags);
                }
                break;
                // TODO: disks
                
            default:
                break;
        }
    }
    catch (...) {
        return ArchivePtr();
    }
        
    for (auto file : archive->files) {
	/* TODO: file_state(file) = FILE_UNKNOWN; */
	file_where(&file) = FILE_INGAME;
    }

    if (!(archive->flags & ARCHIVE_FL_NOCACHE)) {
        archive->id = ++next_id;
        archive_by_id[archive->id] = archive;
        
        if (IS_EXTERNAL(archive->where)) {
	    memdb_file_insert_archive(archive.get());
        }
    }

    archive->flags = ((flags | _archive_global_flags) & (ARCHIVE_FL_MASK | ARCHIVE_FL_HASHTYPES_MASK));

    return archive;
}

Archive::Archive(const std::string &name_, filetype_t ft, where_t where_, int flags_) : id(0), name(name_), filetype(ft), where(where_), flags(0), cache_db(NULL), cache_changed(false), mtime(0), size(0), modified(false) { }

ArchivePtr Archive::open_toplevel(const std::string &name, filetype_t filetype, where_t where, int flags) {
    ArchivePtr a = open(name + "/", filetype, where, flags | ARCHIVE_FL_TOP_LEVEL_ONLY);

    if (a && a->files.empty()) {
        return NULL;
    }

    return a;
}


bool
archive_read_infos(Archive *a) {
    array_t *files_cache = NULL;

    a->cache_db = dbh_cache_get_db_for_archive(archive_name(a));
    a->cache_changed = false;
    if (a->cache_db) {
	a->cache_id = dbh_cache_get_archive_id(a->cache_db, archive_name(a));

	if (a->cache_id > 0) {
	    files_cache = array_new(sizeof(file_t));
	    if (!dbh_cache_read(a->cache_db, archive_name(a), files_cache)) {
		array_free(files_cache, reinterpret_cast<void (*)(void *)>(file_finalize));
		return false;
	    }

	    switch (archive_cache_is_up_to_date(a)) {
	    case -1:
		array_free(files_cache, reinterpret_cast<void (*)(void *)>(file_finalize));
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
	array_free(files_cache, reinterpret_cast<void (*)(void *)>(file_finalize));
	a->cache_changed = true;
	return false;
    }

    if (!merge_files(a, files_cache)) {
	array_free(files_cache, reinterpret_cast<void (*)(void *)>(file_finalize));
	return false;
    }

    if (files_cache) {
	array_free(files_cache, reinterpret_cast<void (*)(void *)>(file_finalize));
	a->cache_changed = true;
    }
    else if (archive_num_files(a) > 0) {
	a->cache_changed = true;
    }

    return true;
}


int
archive_refresh(Archive *a) {
    archive_close(a);
    array_truncate(archive_files(a), 0, reinterpret_cast<void (*)(void *)>(file_finalize));
    archive_read_infos(a);

    return 0;
}


int
archive_register_cache_directory(const char *name) {
    return dbh_cache_register_cache_directory(name);
}

bool
archive_is_empty(const Archive *a) {
    int i;

    for (i = 0; i < archive_num_files(a); i++)
	if (file_where(archive_file(a, i)) != FILE_DELETED)
	    return false;

    return true;
}


static int
archive_cache_is_up_to_date(Archive *a) {
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
cmp_file_by_name(const file_t *f, const char *name) {
    return strcmp(file_name(f), name);
}


bool Archive::get_hashes(File *f, size_t len, struct hashes *h) {
    hashes_update_t *hu;
    unsigned char buf[BUFSIZE];
    size_t n;

    hu = hashes_update_new(h);

    while (len > 0) {
	n = static_cast<size_t>(len) > sizeof(buf) ? sizeof(buf) : len;

	if (f->read(buf, n) != n) {
	    hashes_update_discard(hu);
            return false;
	}

	hashes_update(hu, buf, n);
	len -= n;
    }

    hashes_update_final(hu);

    return true;
}


static bool
merge_files(Archive *a, array_t *files) {
    int i, idx;

    for (i = 0; i < array_length(archive_files(a)); i++) {
	file_t *file = archive_file(a, i);

	if (files && (idx = array_find(files, file_name(file), reinterpret_cast<int (*)(const void *, const void *)>(cmp_file_by_name))) >= 0) {
	    file_t *cfile = static_cast<file_t *>(array_get(files, idx));
	    if (file_mtime(file) == file_mtime(cfile) && file_compare_nsc(file, cfile)) {
		hashes_copy(file_hashes(file), file_hashes(cfile));
	    }
	}
	if (!hashes_has_type(file_hashes(file), HASHES_TYPE_CRC)) {
	    if (archive_file_compute_hashes(a, i, HASHES_TYPE_ALL) < 0) {
		file_status_(file) = STATUS_BADDUMP;
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
replace_files(Archive *a, array_t *files) {
    array_free(archive_files(a), reinterpret_cast<void (*)(void *)>(file_finalize));
    archive_files(a) = files;
}
