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


bool Archive::read_infos() {
    std::vector<file_t> files_cache;

    cache_db = dbh_cache_get_db_for_archive(name.c_str());
    cache_id = cache_db ? dbh_cache_get_archive_id(cache_db, name.c_str()) : 0;
    cache_changed = false;
    if (cache_id > 0) {
        if (!dbh_cache_read(cache_db, name, &files_cache)) {
            return false;
        }

        switch (cache_is_up_to_date()) {
            case -1:
                return false;
                
            case 0:
                cache_changed = true;
		break;

	    case 1:
                files = files_cache;
		return true;
	    }
    }

    if (!read_infos_xxx()) {
        cache_changed = true;
	return false;
    }

    merge_files(files_cache);

    return true;
}


void Archive::refresh() {
    close();
    files.clear();
    read_infos();
}


int Archive::register_cache_directory(const std::string &name) {
    return dbh_cache_register_cache_directory(name.c_str());
}

bool Archive::is_empty() const {
    for (auto &file : files) {
        if (file_where(&file) != FILE_DELETED) {
            return false;
        }
    }

    return true;
}


int Archive::cache_is_up_to_date() {
    get_last_update();

    if (mtime == 0 && size == 0) {
        return 0;
    }

    time_t mtime_cache;
    off_t size_cache;

    if (!dbh_cache_get_archive_last_change(cache_db, cache_id, &mtime_cache, &size_cache)) {
	return -1;
    }

    return (mtime_cache == mtime && size_cache == size);
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


void Archive::merge_files(const std::vector<file_t> &files_cache) {
    for (uint64_t i = 0; i < files.size(); i++) {
        auto &file = files[i];
        
        auto it = std::find_if(files_cache.cbegin(), files_cache.cend(), [&file](const file_t &file_cache){ return file.name == file_cache.name; });
        if (it != files_cache.cend()) {
            if (file_mtime(&file) == file_mtime(&(*it)) && file_compare_nsc(&file, &(*it))) {
                hashes_copy(file_hashes(&file), file_hashes(&(*it)));
            }
            else {
                cache_changed = true;
            }
        }
        else {
            cache_changed = true;
        }
        
        if (!hashes_has_type(file_hashes(&file), HASHES_TYPE_CRC)) {
            if (!file_compute_hashes(i, HASHES_TYPE_ALL)) {
                file_status_(&file) = STATUS_BADDUMP;
                if (it == files_cache.cend() || file_status_(&(*it)) != STATUS_BADDUMP) {
                    cache_changed = true;
                }
                continue;
            }
            if (detector) {
                file_match_detector(i);
            }
            cache_changed = true;
        }
    }
    
    if (files.size() != files_cache.size()) {
        cache_changed = true;
    }
}

std::optional<size_t> Archive::file_index(const file_t *file) const {
    for (size_t index = 0; index < files.size(); index++) {
        if (&files[index] == file) {
            return index;
        }
    }
    
    return {};
}
