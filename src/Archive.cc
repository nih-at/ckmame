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
#include <utility>

#include "config.h"
#include "ArchiveDir.h"
#include "ArchiveImages.h"
#ifdef HAVE_LIBARCHIVE
#include "ArchiveLibarchive.h"
#endif
#include "ArchiveZip.h"
#include "Detector.h"
#include "CkmameDB.h"
#include "Exception.h"
#include "file_util.h"
#include "globals.h"
#include "MemDB.h"
#include "RomDB.h"
#include "CkmameCache.h"

#define BUFSIZE 8192

//#define DEBUG_LC

bool Archive::read_only_mode = false;

uint64_t ArchiveContents::next_id = 0;
std::unordered_map<ArchiveContents::TypeAndName, std::weak_ptr<ArchiveContents>> ArchiveContents::archive_by_name;
std::unordered_map<uint64_t, ArchiveContentsPtr> ArchiveContents::archive_by_id;

ArchiveContents::ArchiveContents(ArchiveType type_, std::string name_, filetype_t filetype_, where_t where_, int flags_, std::string filename_extension_) :
    id(0),
    name(std::move(name_)),
    filetype(filetype_),
    where(where_),
    cache_id(0),
    flags(flags_),
    mtime(0),
    size(0),
    archive_type(type_),
    filename_extension(std::move(filename_extension_)) { }

Archive::Archive(ArchiveContentsPtr contents_) :
    contents(std::move(contents_)),
    files(contents->files),
    name(contents->name),
    filetype(contents->filetype),
    where(contents->where),
    cache_changed(false),
    modified(false) {
        changes.resize(files.size());
    }

Archive::Archive(ArchiveType type, const std::string &name_, filetype_t ft, where_t where_, int flags_) : Archive(std::make_shared<ArchiveContents>(type, name_, ft, where_, flags_)) { }

ArchivePtr Archive::open(const ArchiveContentsPtr& contents) {
    ArchivePtr archive;
    
    if (contents->open_archive.expired()) {
        switch (contents->archive_type) {
            case ARCHIVE_LIBARCHIVE:
#ifdef HAVE_LIBARCHIVE
                archive = std::make_shared<ArchiveLibarchive>(contents);
#else
                return nullptr;
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
                return nullptr;
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
        return nullptr;
    }
    
    return open(contents);
}


int Archive::close() {
    int ret;
    output.set_error_archive(name);

    ret = commit();

    return close_xxx() | ret;
}


void Archive::ensure_valid_archive() {
    if (check()) {
        return;
    }

    /* opening the archive failed, rename it and create new one */
    
    auto new_name = ::make_unique_name(name, ".broken");
    
    output.message_verbose("rename broken archive '%s' to '%s'", name.c_str(), new_name.c_str());
    if (!rename_or_move(name, new_name)) {
        throw(Exception("can't rename file")); // TODO: rename_or_move should throw
    }
    if (!check()) {
        throw(Exception("can't create archive '" + name + "'")); // TODO: details
    }
}


int Archive::file_compare_hashes(uint64_t index, const Hashes *hashes) {
    auto &file_hashes = files[index].hashes;

    file_ensure_hashes(index, hashes->get_types());

    if (files[index].broken) {
        return Hashes::NOCOMMON;
    }

    return file_hashes.compare(*hashes);
}


bool Archive::file_ensure_hashes(uint64_t idx, size_t detector_id, int hashtypes) {
    auto &file = files[idx];
    
    if (file.has_all_hashes(detector_id, hashtypes)) {
	return true;
    }
    
    if (file.broken) {
        return false;
    }

    if (detector_id == 0) {
	Hashes hashes;
	hashes.add_types(Hashes::TYPE_ALL);


        ZipSourcePtr f;

	try {
            f = get_source(idx);
	    f->open();
	} catch (Exception &e) {
	    output.error("%s: %s: can't open: %s", name.c_str(), file.name.c_str(), e.what());
	    file.broken = true;
	    return false;
	}

	switch (get_hashes(f.get(), file.hashes.size, true, &hashes)) {
	case OK:
	    break;

	case READ_ERROR:
	    output.error("%s: %s: can't compute hashes: %s", name.c_str(), file.name.c_str(), strerror(errno));
	    file.broken = true;
	    return false;

	case CRC_ERROR:
	    output.error("%s: %s: CRC error: %08x != %08x", name.c_str(), file.name.c_str(), hashes.crc, file.hashes.crc);
	    file.broken = true;
	    return false;
	}

	file.hashes.set_hashes(hashes);
    }
    else {
	if (!compute_detector_hashes(idx, db->detectors)) {
	    return false;
	}
    }
    cache_changed = true;

    return true;
}


std::optional<size_t> Archive::file_find_offset(size_t index, size_t size, const Hashes *hashes) {
    Hashes hashes_part;

    hashes_part.add_types(hashes->get_types());

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


std::optional<size_t> Archive::file_index_by_name(const std::string &filename) const {
    auto index = contents->file_index_by_name(filename);
    
    if (index.has_value() && changes[index.value()].status == Change::DELETED) {
        return {};
    }

    return index;
}


std::string Archive::make_unique_name_in_archive(const std::string &filename) {
    if (!file_index_by_name(filename).has_value()) {
        return filename;
    }

    std::string ext = std::filesystem::path(filename).extension().string();

    for (int i = 0; i < 1000; i++) {
	char n[5];
	snprintf(n, sizeof(n), "-%03d", i);
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
                if (configuration.roms_zipped) {
                    auto extension = std::filesystem::path(archive_name).extension();
                    if (strcasecmp(extension.c_str(), ".zip") == 0) {
                        archive = std::make_shared<ArchiveZip>(archive_name, filetype, where, flags);
                    }
#ifdef HAVE_LIBARCHIVE
                    else if (strcasecmp(extension.c_str(), ".7z") == 0) {
                        archive = std::make_shared<ArchiveLibarchive>(archive_name, filetype, where, flags);
                    }
#endif
                    else {
                        return nullptr;
                    }
                }
                else {
                    archive = std::make_shared<ArchiveDir>(archive_name, filetype, where, flags);
                }
                break;
                
            case TYPE_DISK:
                archive = std::make_shared<ArchiveImages>(archive_name, filetype, where, flags);
                break;
                
            default:
                return nullptr;
        }
    }
    catch (...) {
        return {};
    }
    
    archive->contents->open_archive = archive;
    archive->contents->flags = ((flags  & (ARCHIVE_FL_MASK | ARCHIVE_FL_HASHTYPES_MASK)) | (read_only_mode ? ARCHIVE_FL_RDONLY : 0));
    
    if (!archive->read_infos() && (flags & ARCHIVE_FL_CREATE) == 0) {
        return {};
    }
    
    ArchiveContents::enter_in_maps(archive->contents);

    return archive;
}


ArchivePtr Archive::open_toplevel(const std::string &name, filetype_t filetype, where_t where, int flags) {
    ArchivePtr a = open(name, filetype, where, flags | ARCHIVE_FL_TOP_LEVEL_ONLY);

    if (a && a->files.empty()) {
        return nullptr;
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
                    changes.resize(files.size());
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


bool Archive::is_empty() const {
    return std::all_of(changes.begin(), changes.end(), [](const Change &change) {
	return change.status == Change::DELETED;
    });
}


int ArchiveContents::is_cache_up_to_date() const {
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
                file.detector_hashes = it->detector_hashes;
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

bool ArchiveContents::read_infos_from_cachedb(std::vector<File> *cached_files) {
    if (!configuration.roms_zipped && filetype == TYPE_DISK) {
        cache_db = nullptr;
        cache_id = -1;
        return true;
    }
    
    cache_db = ckmame_cache->get_db_for_archive(name);
    cache_id = cache_db ? cache_db->get_archive_id(name, filetype) : 0;

    if (cache_id > 0) {
        try {
            cache_db->read_files(cache_id, cached_files);
        }
        catch (Exception &exception) {
            return false;
        }
    }

    return true;
}

void ArchiveContents::enter_in_maps(const ArchiveContentsPtr &contents) {
    if (!(contents->flags & ARCHIVE_FL_NOCACHE)) {
        contents->id = ++next_id;
        archive_by_id[contents->id] = contents;
        
        if (IS_EXTERNAL(contents->where)) {
            memdb->insert_archive(contents.get());
        }
    }

    archive_by_name[TypeAndName(contents->filetype, contents->name)] = contents;
}

ArchiveContentsPtr ArchiveContents::by_id(uint64_t id) {
    auto it = archive_by_id.find(id);
    
    if (it == archive_by_id.end()) {
        return nullptr;
    }
    
    return it->second;
}

ArchiveContentsPtr ArchiveContents::by_name(filetype_t filetype, const std::string &name) {
    auto it = archive_by_name.find(TypeAndName(filetype, name));
    
    if (it == archive_by_name.end() || it->second.expired()) {
        return nullptr;
    }
    
    return it->second.lock();
}


void ArchiveContents::clear_cache() {
    archive_by_name.clear();
    archive_by_id.clear();
    next_id = 0;
}


std::optional<size_t> ArchiveContents::file_index_by_name(const std::string &filename) const {
    for (size_t i = 0; i < files.size(); i++) {
        auto &file = files[i];
        
        if (filename == file.name) {
            return i;
        }
    }

    return {};
}

bool Archive::compute_detector_hashes(const std::unordered_map<size_t, DetectorPtr> &detectors) {
    auto got_new_hashes = false;
    
    for (size_t index = 0; index < files.size(); index++) {
        auto &file = files[index];
        std::unordered_map<size_t, DetectorPtr> missing_detectors;
        
        for (const auto &pair : detectors) {
            if (file.detector_hashes.find(pair.first) == file.detector_hashes.end()) {
                missing_detectors[pair.first] = pair.second;
            }
        }
        
        if (missing_detectors.empty()) {
            continue;
        }
        
        if (compute_detector_hashes(index, missing_detectors)) {
            got_new_hashes = true;
        }
    }
    
    if (got_new_hashes) {
        cache_changed = true;
    }
    return got_new_hashes;
}


bool Archive::compute_detector_hashes(size_t index, const std::unordered_map<size_t, DetectorPtr> &detectors) {
    auto &file = files[index];
    
    if (file.broken) {
        return false;
    }
    
    auto data = std::vector<uint8_t>(file.get_size(0));

    try {
        auto source = get_source(index);
        if (!source) {
            throw Exception("can't open: %s", strerror(errno));
        }
        source->open();
        source->read(data.data(), data.size());
    }
    catch (std::exception &e) {
        output.error("%s: %s: can't compute hashes: %s", name.c_str(), file.name.c_str(), e.what());
        file.broken = true;
            
        return false;
    }

    auto ok = Detector::compute_hashes(data, &file, detectors);
    if (ok) {
	memdb->update_file(contents.get(), index);
    }
    return ok;
}


bool ArchiveContents::has_all_detector_hashes(const std::unordered_map<size_t, DetectorPtr> &detectors) {
    for (const auto &pair : detectors) {
        for (const auto &file: files) {
            if (file.detector_hashes.find(pair.first) == file.detector_hashes.end()) {
                return false;
            }
        }
    }
    
    return true;
}


bool Archive::compare_size_hashes(size_t index, size_t detector_id, const FileData *rom) {
    auto &file = files[index];
    
    auto ok = false;

    if (rom->compare_size(file)) {
        file_ensure_hashes(index, 0, rom->hashes.get_types());
        if (rom->compare_size_hashes(file)) {
            if (file.size_hashes_are_set(true) || file_compare_hashes(index, &rom->hashes) == 0) {
                ok = true;
            }
        }
    }
    if (!ok) {
        if (detector_id > 0) {
            compute_detector_hashes(db->detectors);
            if (rom->hashes.compare(file.get_hashes(detector_id)) == Hashes::MATCH) {
                ok = true;
            }
        }
    }
    
    return ok;
}
