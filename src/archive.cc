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


#include <algorithm>

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

uint64_t Archive::next_id;
std::unordered_map<std::string, std::weak_ptr<Archive>> Archive::archive_by_name;
std::unordered_map<uint64_t, ArchivePtr> Archive::archive_by_id;

int Archive::close() {
    int ret;
    seterrinfo("", name);

    ret = commit();

    return close_xxx() | ret;
}

int Archive::file_compare_hashes(uint64_t index, const Hashes *hashes) {
    auto &file_hashes = files[index].hashes;

    if (!file_hashes.has_all_types(*hashes)) {
        file_compute_hashes(index, hashes->types | romdb_hashtypes(db, TYPE_ROM));
    }

    if (files[index].status != STATUS_OK) {
        return Hashes::NOCOMMON;
    }

    return file_hashes.compare(*hashes);
}


bool Archive::file_compute_hashes(uint64_t idx, int hashtypes) {
    Hashes hashes;
    auto &file = files[idx];

    if (file.hashes.has_all_types(hashtypes)) {
	return true;
    }

    hashes.types = Hashes::TYPE_ALL;

    auto f = file_open(idx);
    if (!f) {
	myerror(ERRDEF, "%s: %s: can't open: %s", name.c_str(), file.name.c_str(), strerror(errno));
	file.status = STATUS_BADDUMP;
	return false;
    }

    if (!get_hashes(f.get(), file.size, &hashes)) {
	myerror(ERRDEF, "%s: %s: can't compute hashes: %s", name.c_str(), file.name.c_str(), strerror(errno));
	file.status = STATUS_BADDUMP;
	return false;
    }

    if (file.hashes.types & hashtypes & Hashes::TYPE_CRC) {
	if (file.hashes.crc != hashes.crc) {
            myerror(ERRDEF, "%s: %s: CRC error: %x != %x", name.c_str(), file.name.c_str(), hashes.crc, file.hashes.crc);
            file.status = STATUS_BADDUMP;
	    return false;
	}
    }
    file.hashes = hashes;

    cache_changed = true;

    return true;
}


std::optional<size_t> Archive::file_find_offset(size_t idx, size_t size, const Hashes *hashes) {
    Hashes hashes_part;

    hashes_part.types = hashes->types;

    auto &file = files[idx];

    auto f = file_open(idx);
    if (!f) {
        file.status = STATUS_BADDUMP;
        return {};
    }

    auto found = false;
    size_t offset = 0;
    while (offset + size <= file.size) {
        if (!get_hashes(f.get(), size, &hashes_part)) {
            file.status = STATUS_BADDUMP;
            return {};
	}

	if (hashes->compare(hashes_part) == Hashes::MATCH) {
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


std::optional<size_t> Archive::file_index_by_hashes(const Hashes *hashes) const {
    if (hashes->empty()) {
        return {};
    }

    for (size_t i = 0; i < files.size(); i++) {
        auto &file = files[i];

	if (hashes->compare(file.hashes) == Hashes::MATCH) {
	    if (file.where == FILE_DELETED) {
                return {};
	    }
	    return i;
	}
    }

    return {};
}


std::optional<size_t> Archive::file_index_by_name(const std::string &filename) const {
    for (size_t i = 0; i < files.size(); i++) {
        auto &file = files[i];

        if (filename == file.name) {
            if (file.where == FILE_DELETED) {
                return {};
            }
	    return i;
	}
    }

    return {};
}


void Archive::file_match_detector(uint64_t index) {
    auto &file = files[index];

    auto fp = file_open(index);
    if (!fp) {
        myerror(ERRZIP, "%s: can't open: %s", file.name.c_str(), strerror(errno));
        file.status = STATUS_BADDUMP;
    }
    
    detector->execute(&file, Archive::file_read_c, fp.get());
}


int64_t Archive::file_read_c(void *fp, void *data, uint64_t length) {
    auto file = static_cast<Archive::ArchiveFile *>(fp);
    
    return file->read(data, length);
}

int _archive_global_flags;

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
        strcpy(unique, filename.c_str());
	p = unique + strlen(unique);
	p[4] = '\0';
    }
    else {
        strncpy(unique, filename.c_str(), ext_index);
        p = unique + ext_index;
	strcpy(p + 4, filename.c_str() + ext_index);
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
        if (!it->second.expired()) {
            return it->second.lock();
        }
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
    
    archive->flags = ((flags | _archive_global_flags) & (ARCHIVE_FL_MASK | ARCHIVE_FL_HASHTYPES_MASK));
    
    if (!archive->read_infos() && (flags & ARCHIVE_FL_CREATE) == 0) {
        return ArchivePtr();
    }
        
    for (auto &file : archive->files) {
	/* TODO: file.state = FILE_UNKNOWN; */
	file.where = FILE_INGAME;
    }

    if (!(archive->flags & ARCHIVE_FL_NOCACHE)) {
        archive->id = ++next_id;
        archive_by_id[archive->id] = archive;
        
        if (IS_EXTERNAL(archive->where)) {
	    memdb_file_insert_archive(archive.get());
        }
    }

    archive_by_name[name] = archive;

    return archive;
}


void Archive::flush_cache() {
    archive_by_id.clear();
    archive_by_name.clear();
}


Archive::Archive(const std::string &name_, filetype_t ft, where_t where_, int flags_) : id(0), name(name_), filetype(ft), where(where_), flags(0), cache_db(NULL), cache_changed(false), mtime(0), size(0), modified(false) { }

ArchivePtr Archive::open_toplevel(const std::string &name, filetype_t filetype, where_t where, int flags) {
    ArchivePtr a = open(name, filetype, where, flags | ARCHIVE_FL_TOP_LEVEL_ONLY);

    if (a && a->files.empty()) {
        return NULL;
    }

    return a;
}


bool Archive::read_infos() {
    std::vector<File> files_cache;

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
        if (file.where != FILE_DELETED) {
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

    return (mtime_cache == mtime && static_cast<size_t>(size_cache) == size);
}


bool Archive::get_hashes(ArchiveFile *f, size_t len, Hashes *h) {
    unsigned char buf[BUFSIZE];
    size_t n;

    auto hu = Hashes::Update(h);

    while (len > 0) {
	n = static_cast<size_t>(len) > sizeof(buf) ? sizeof(buf) : len;

	if (f->read(buf, n) != static_cast<int64_t>(n)) {
            return false;
	}

	hu.update(buf, n);
	len -= n;
    }

    hu.end();

    return true;
}


void Archive::merge_files(const std::vector<File> &files_cache) {
    for (uint64_t i = 0; i < files.size(); i++) {
        auto &file = files[i];
        
        auto it = std::find_if(files_cache.cbegin(), files_cache.cend(), [&file](const File &file_cache){ return file.name == file_cache.name; });
        if (it != files_cache.cend()) {
            if (file.mtime == (*it).mtime && file.compare_name_size_crc(*it)) {
                file.hashes = (*it).hashes;
            }
            else {
                cache_changed = true;
            }
        }
        else {
            cache_changed = true;
        }
        
        if (!file.hashes.has_type(Hashes::TYPE_CRC)) {
            if (!file_compute_hashes(i, Hashes::TYPE_ALL)) {
                file.status = STATUS_BADDUMP;
                if (it == files_cache.cend() || (*it).status != STATUS_BADDUMP) {
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

std::optional<size_t> Archive::file_index(const File *file) const {
    for (size_t index = 0; index < files.size(); index++) {
        if (&files[index] == file) {
            return index;
        }
    }
    
    return {};
}
