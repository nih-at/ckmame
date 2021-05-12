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

#include "Archive.h"

#include <algorithm>
#include <cerrno>
#include <cstring>

#include "config.h"
#include "ArchiveDir.h"
#include "ArchiveImages.h"
#ifdef HAVE_LIBARCHIVE
#include "ArchiveLibarchive.h"
#endif
#include "ArchiveZip.h"
#include "Detector.h"
#include "CkmameDB.h"
#include "error.h"
#include "Exception.h"
#include "file_util.h"
#include "fix.h"
#include "fix_util.h"
#include "globals.h"
#include "MemDB.h"

#define BUFSIZE 8192

//#define DEBUG_LC

uint64_t ArchiveContents::next_id;
std::unordered_map<ArchiveContents::TypeAndName, std::weak_ptr<ArchiveContents>> ArchiveContents::archive_by_name;
std::unordered_map<uint64_t, ArchiveContentsPtr> ArchiveContents::archive_by_id;

ArchiveContents::ArchiveContents(ArchiveType type_, const std::string &name_, filetype_t filetype_, where_t where_, int flags_, const std::string &filename_extension_) :
    id(0),
    name(name_),
    filetype(filetype_),
    where(where_),
    cache_id(0),
    flags(flags_),
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
    modified(false) {
        changes.resize(files.size());
    }

Archive::Archive(ArchiveType type, const std::string &name_, filetype_t ft, where_t where_, int flags_) : Archive(std::make_shared<ArchiveContents>(type, name_, ft, where_, flags_)) { }

ArchivePtr Archive::open(ArchiveContentsPtr contents) {
    ArchivePtr archive;
    
    if (contents->open_archive.expired()) {
        switch (contents->archive_type) {
            case ARCHIVE_LIBARCHIVE:
#ifdef HAVE_LIBARCHIVE
                archive = std::make_shared<ArchiveLibarchive>(contents);
#else
                return NULL;
#endif
                break;
                
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

    if (files[index].broken) {
        return Hashes::NOCOMMON;
    }

    return file_hashes.compare(*hashes);
}


bool Archive::file_ensure_hashes(uint64_t idx, int hashtypes) {
    auto &file = files[idx];
    
    if (file.hashes.has_all_types(hashtypes)) {
	return true;
    }

    Hashes hashes;
    hashes.types = Hashes::TYPE_ALL;

    auto f = get_source(idx);
    
    if (!f) {
        // TODO: move error message to get_source()
	myerror(ERRDEF, "%s: %s: can't open: %s", name.c_str(), file.name.c_str(), strerror(errno));
	file.broken = true;
	return false;
    }
    
    try {
        f->open();
    }
    catch (Exception &e) {
        myerror(ERRDEF, "%s: %s: can't open: %s", name.c_str(), file.name.c_str(), e.what());
        file.broken = true;
        return false;
    }

    switch (get_hashes(f.get(), file.hashes.size, true, &hashes)) {
        case OK:
            break;

        case READ_ERROR:
            myerror(ERRDEF, "%s: %s: can't compute hashes: %s", name.c_str(), file.name.c_str(), strerror(errno));
            file.broken = true;
            return false;

        case CRC_ERROR:
            myerror(ERRDEF, "%s: %s: CRC error: %08x != %08x", name.c_str(), file.name.c_str(), hashes.crc, file.hashes.crc);
            file.broken = true;
            return false;
    }

    file.hashes.set_hashes(hashes);

    cache_changed = true;

    return true;
}


std::optional<size_t> Archive::file_find_offset(size_t index, size_t size, const Hashes *hashes) {
    Hashes hashes_part;

    hashes_part.types = hashes->types;

    auto &file = files[index];

    try {
        auto source = get_source(index);

        source->open();
        
        auto found = false;
        size_t offset = 0;
        while (offset + size <= file.hashes.size) {
            if (get_hashes(source.get(), size, offset + size == file.hashes.size, &hashes_part) != OK) {
                file.broken = true;
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
    }
    catch (Exception &e) {
        file.broken = true;
    }

    return {};
}


std::optional<size_t> Archive::file_index_by_hashes(const Hashes *hashes) const {
    if (hashes->empty()) {
        return {};
    }

    for (size_t i = 0; i < files.size(); i++) {
        auto &file = files[i];
        auto &change = changes[i];

	if (hashes->compare(file.hashes) == Hashes::MATCH) {
	    if (change.status == Change::DELETED) {
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
        auto &change = changes[i];

        if (filename == file.name) {
            if (change.status == Change::DELETED) {
                return {};
            }
	    return i;
	}
    }

    return {};
}


void Archive::file_match_detector(uint64_t index, Detector *detector) {
    auto &file = files[index];

    try {
        auto source = get_source(index);
        if (!source) {
            throw Exception("%s", strerror(errno));
        }
        source->open();
        detector->execute(&file, Archive::file_read_c, source.get());
    }
    catch (Exception &e) {
        myerror(ERRZIP, "%s: can't open: %s", file.name.c_str(), e.what());
        file.broken = true;
    }
}


int64_t Archive::file_read_c(void *fp, void *data, uint64_t length) {
    auto source = static_cast<ZipSource *>(fp);
    
    return static_cast<int64_t>(source->read(data, length));
}

int _archive_global_flags;

void
archive_global_flags(int fl, bool setp) {
    if (setp)
	_archive_global_flags |= fl;
    else
	_archive_global_flags &= ~fl;
}


std::string Archive::make_unique_name_in_archive(const std::string &filename) {
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
    std::string archive_name;
    
    if (name[name.length() - 1] == '/') {
        archive_name = name.substr(0, name.length() - 1);
        flags |= ARCHIVE_FL_TOP_LEVEL_ONLY;
    }
    else {
        archive_name = name;
    }
    auto contents = ArchiveContents::by_name(filetype, archive_name);

    if (contents) {
        return open(contents);
    }

    ArchivePtr archive;
    
    try {
        switch (filetype) {
            case TYPE_ROM:
                if (roms_unzipped) {
                    archive = std::make_shared<ArchiveDir>(archive_name, filetype, where, flags);
                }
                else {
                    auto extension = std::filesystem::path(archive_name).extension();
                    if (strcasecmp(extension.c_str(), ".zip") == 0) {
                        archive = std::make_shared<ArchiveZip>(archive_name, filetype, where, flags);
                    }
#ifdef HAVE_LIBARCHIVE
                    else if (strcasecmp(extension.c_str(), ".7z") == 0) {
                        archive = std::make_shared<ArchiveLibarchive>(archive_name, filetype, where, flags);
                    }
#endif
                }
                break;
                
            case TYPE_DISK:
                archive = std::make_shared<ArchiveImages>(archive_name, filetype, where, flags);
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
    
    ArchiveContents::enter_in_maps(archive->contents);

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
                // For directories, mtime doesn't change for all changes of files within that directory, so we always have to rescan.
                if (contents->size != 0) {
                    files = files_cache;
                    return true;
                }
                break;
	    }
    }

    if (!read_infos_xxx()) {
        cache_changed = true;
	return false;
    }

    merge_files(files_cache);

    changes.resize(files.size());

    return true;
}


void Archive::refresh() {
    close();
    files.clear();
    read_infos();
}


bool Archive::is_empty() const {
    for (auto &change : changes) {
        if (change.status != Change::DELETED) {
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

    try {
        cache_db->get_last_change(cache_id, &mtime_cache, &size_cache);
    }
    catch (Exception &exception) {
	return -1;
    }

    return (mtime_cache == mtime && static_cast<size_t>(size_cache) == size);
}


Archive::GetHashesStatus Archive::get_hashes(ZipSource *source, uint64_t length, bool eof, Hashes *hashes) {
    unsigned char buf[BUFSIZE];

    try {
        auto hu = Hashes::Update(hashes);

        while (length > 0) {
            uint64_t n = std::min(length, static_cast<uint64_t>(sizeof(buf)));
            if (source->read(buf, n) != n) {
                throw Exception();
            }

            hu.update(buf, n);
            length -= n;
        }

        hu.end();
    }
    catch (Exception &e) {
        return READ_ERROR;
    }

    if (eof) {
        try {
            source->read(buf, 1);
        }
        catch (Exception &e) {
            return CRC_ERROR;
        }
    }

    return OK;
}


void Archive::merge_files(const std::vector<File> &files_cache) {
    for (uint64_t i = 0; i < files.size(); i++) {
        auto &file = files[i];
        
        file.filename_extension = contents->filename_extension;
        auto it = std::find_if(files_cache.cbegin(), files_cache.cend(), [&file](const File &file_cache){ return file.name == file_cache.name; });
        if (it != files_cache.cend()) {
            if (file.mtime == (*it).mtime && file.compare_size_hashes(*it)) {
                file.hashes.merge((*it).hashes);
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
                file.broken = true;
                if (it == files_cache.cend() || !(*it).broken) {
                    cache_changed = true;
                }
                continue;
            }
#if 0
            if (detector) {
                file_match_detector(i);
            }
#endif
            cache_changed = true;
        }
    }
    
    if (files.size() != files_cache.size()) {
        cache_changed = true;
    }
}

std::optional<size_t> Archive::file_index(const FileData *file) const {
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
    
    cache_db = CkmameDB::get_db_for_archvie(name);
    cache_id = cache_db ? cache_db->get_archive_id(name) : 0;

    if (cache_id > 0) {
        try {
            cache_db->read_files(cache_id, files);
        }
        catch (Exception &exception) {
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
            MemDB::insert_archive(contents.get());
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
