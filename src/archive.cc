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

#include "archive.h"

#include <algorithm>
#include <cerrno>
#include <cstring>

#include "ArchiveDir.h"
#include "ArchiveImages.h"
#include "ArchiveZip.h"
#include "dbh_cache.h"
#include "error.h"
#include "file_util.h"
#include "fix_util.h"
#include "globals.h"
#include "memdb.h"

#define BUFSIZE 8192

//#define DEBUG_LC

uint64_t ArchiveContents::next_id;
std::unordered_map<ArchiveContents::TypeAndName, std::weak_ptr<ArchiveContents>> ArchiveContents::archive_by_name;
std::unordered_map<uint64_t, ArchiveContentsPtr> ArchiveContents::archive_by_id;

ArchiveContents::ArchiveContents(ArchiveType type_, const std::string &name_, filetype_t filetype_, where_t where_, int flags_, std::string *filename_extension_) :
    id(0),
    name(name_),
    filetype(filetype_),
    where(where_),
    cache_id(0),
    flags(0),
    mtime(0),
    size(0),
    archive_type(type_),
    filename_extension(filename_extension_) { }

Archive::Archive(ArchiveContentsPtr contents_) :
    contents(contents_),
    files(contents->files),
    name(contents->name),
    filetype(contents->filetype),
    where(contents->where),
    cache_changed(false),
    modified(false) { }

Archive::Archive(ArchiveType type, const std::string &name_, filetype_t ft, where_t where_, int flags_) : Archive(std::make_shared<ArchiveContents>(type, name_, ft, where_, flags_)) { }

ArchivePtr Archive::open(ArchiveContentsPtr contents) {
    ArchivePtr archive;
    
    if (contents->open_archive.expired()) {
        switch (contents->archive_type) {
            case ARCHIVE_ZIP:
                archive = std::make_shared<ArchiveZip>(contents);
                break;
                
            case ARCHIVE_DIR:
                archive = std::make_shared<ArchiveDir>(contents);
                break;
                
            case ARCHIVE_IMAGES:
                archive = std::make_shared<ArchiveImages>(contents);
                break;
                
            default:
                return NULL;
        }

        //printf("# reopening %s\n", archive->name.c_str());
        contents->open_archive = archive;
    }
    else {
        //printf("# already open %s\n", archive->name.c_str());
        archive = contents->open_archive.lock();
    }
    
    return archive;
}

ArchivePtr Archive::by_id(uint64_t id) {
    auto contents = ArchiveContents::by_id(id);
    
    if (!contents) {
        return NULL;
    }
    
    return open(contents);
}


int Archive::close() {
    int ret;
    seterrinfo("", name);

    ret = commit();

    return close_xxx() | ret;
}


void Archive::ensure_valid_archive() {
    if (check()) {
        return;
    }

    /* opening the archive failed, rename it and create new one */
    
    auto new_name = ::make_unique_name(name, ".broken");
    
    if (fix_options & FIX_PRINT) {
        printf("%s: rename broken archive to '%s'\n", name.c_str(), new_name.c_str());
    }
    if (!rename_or_move(name, new_name)) {
        throw("can't rename file"); // TODO: rename_or_move should throw
    }
    if (!check()) {
        throw("can't create archive '" + name + "'"); // TODO: details
    }
}


int Archive::file_compare_hashes(uint64_t index, const Hashes *hashes) {
    auto &file_hashes = files[index].hashes;

    file_ensure_hashes(index, hashes->types);

    if (files[index].status != STATUS_OK) {
        return Hashes::NOCOMMON;
    }

    return file_hashes.compare(*hashes);
}


bool Archive::file_ensure_hashes(uint64_t idx, int hashtypes) {
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
    if (!file_index_by_name(filename).has_value()) {
        return filename;
    }

    std::string ext = std::filesystem::path(filename).extension();

    for (int i = 0; i < 1000; i++) {
	char n[5];
	sprintf(n, "-%03d", i);
	auto unique = filename.substr(0, filename.length() - ext.length()) + n + ext;

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

    return "";
}

ArchivePtr Archive::open(const std::string &name, filetype_t filetype, where_t where, int flags) {
    auto contents = ArchiveContents::by_name(filetype, name);

    if (contents) {
        return open(contents);
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
                
            case TYPE_DISK:
                archive = std::make_shared<ArchiveImages>(name, filetype, where, flags);
                break;
                
            default:
                return NULL;
        }
    }
    catch (...) {
        return ArchivePtr();
    }
    
    archive->contents->open_archive = archive;
    archive->contents->flags = ((flags | _archive_global_flags) & (ARCHIVE_FL_MASK | ARCHIVE_FL_HASHTYPES_MASK));
    
    if (!archive->read_infos() && (flags & ARCHIVE_FL_CREATE) == 0) {
        return ArchivePtr();
    }
        
    for (auto &file : archive->files) {
	/* TODO: file.state = FILE_UNKNOWN; */
	file.where = FILE_INGAME;
    }

    ArchiveContents::enter_in_maps(archive->contents);

    //printf("# opening %s\n", archive->name.c_str());
    return archive;
}


ArchivePtr Archive::open_toplevel(const std::string &name, filetype_t filetype, where_t where, int flags) {
    ArchivePtr a = open(name, filetype, where, flags | ARCHIVE_FL_TOP_LEVEL_ONLY);

    if (a && a->files.empty()) {
        return NULL;
    }

    return a;
}


bool Archive::read_infos() {
    std::vector<File> files_cache;

    cache_changed = false;
    
    contents->read_infos_from_cachedb(&files_cache);

    if (contents->cache_id > 0) {
        switch (contents->is_cache_up_to_date()) {
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
    return dbh_cache_register_cache_directory(name);
}

bool Archive::is_empty() const {
    for (auto &file : files) {
        if (file.where != FILE_DELETED) {
            return false;
        }
    }

    return true;
}


int ArchiveContents::is_cache_up_to_date() {
    if (open_archive.expired()) {
        return 0;
    }
    
    open_archive.lock()->get_last_update();

    if (mtime == 0 && size == 0) {
        return 0;
    }

    time_t mtime_cache;
    off_t size_cache;

    if (!dbh_cache_get_archive_last_change(cache_db.get(), cache_id, &mtime_cache, &size_cache)) {
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
        
        file.filename_extension = contents->filename_extension;
        auto it = std::find_if(files_cache.cbegin(), files_cache.cend(), [&file](const File &file_cache){ return file.name == file_cache.name; });
        if (it != files_cache.cend()) {
            if (file.mtime == (*it).mtime && file.compare_name_size_hashes(*it)) {
                file.hashes = (*it).hashes;
            }
            else {
                cache_changed = true;
            }
        }
        else {
            cache_changed = true;
        }
        
        if (want_crc() && !file.hashes.has_type(Hashes::TYPE_CRC)) {
            if (!file_ensure_hashes(i, Hashes::TYPE_ALL)) {
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


// MARK: - ArchiveContents

bool ArchiveContents::read_infos_from_cachedb(std::vector<File> *files) {
    if (roms_unzipped && filetype == TYPE_DISK) {
        cache_db = NULL;
        cache_id = -1;
        return true;
    }
    
    cache_db = dbh_cache_get_db_for_archive(name);
    cache_id = cache_db ? dbh_cache_get_archive_id(cache_db.get(), name) : 0;

    if (cache_id > 0) {
        if (!dbh_cache_read(cache_db.get(), name, files)) {
            return false;
        }
    }

    return true;
}

void ArchiveContents::enter_in_maps(ArchiveContentsPtr contents) {
    if (!(contents->flags & ARCHIVE_FL_NOCACHE)) {
        contents->id = ++next_id;
        archive_by_id[contents->id] = contents;
        
        if (IS_EXTERNAL(contents->where)) {
            memdb_file_insert_archive(contents.get());
        }
    }

    archive_by_name[TypeAndName(contents->filetype, contents->name)] = contents;
}

ArchiveContentsPtr ArchiveContents::by_id(uint64_t id) {
    auto it = archive_by_id.find(id);
    
    if (it == archive_by_id.end()) {
        return NULL;
    }
    
    return it->second;
}

ArchiveContentsPtr ArchiveContents::by_name(filetype_t filetype, const std::string &name) {
    auto it = archive_by_name.find(TypeAndName(filetype, name));
    
    if (it == archive_by_name.end() || it->second.expired()) {
        return NULL;
    }
    
    return it->second.lock();
}
