/*
  archive_modify.c -- functions to modify an archive
  Copyright (C) 1999-2015 Dieter Baron and Thomas Klausner

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

#include <cerrno>
#include <cinttypes>
#include <cstring>

#include "CkmameDB.h"
#include "Exception.h"
#include "MemDB.h"
#include "CkmameCache.h"
#include "globals.h"

bool Archive::commit() {
    if (modified) {
        output.set_error_archive(name);

        cache_changed = true;

        if (!commit_xxx()) {
            return false;
	}

        for (size_t index = 0; index < files.size(); index++) {
            auto &change = changes[index];

            switch (change.status) {
                case Change::DELETED:
                    if (is_indexed()) {
                        /* TODO: handle error (how?) */
                        memdb->delete_file(contents.get(), index, is_writable());
                    }

                    if (is_writable()) {
                        files.erase(files.begin() + index);
                        changes.erase(changes.begin() + index);
                        index--;
                    }
		break;

                case Change::ADDED:
                    if (is_indexed()) {
                        /* TODO: handle error (how?) */
                        memdb->insert_file(contents.get(), index);
                    }
                    change.status = Change::EXISTS;
                    break;

                default:
                    break;
	    }
	}
        
        changes.clear();
        changes.resize(files.size());

        commit_cleanup();

        modified = false;
    }
    
    update_cache();
    return true;
}

void Archive::update_cache() {
    if (!cache_changed) {
        return;
    }

    if (contents->cache_db == nullptr && ckmame_cache) {
        contents->cache_db = ckmame_cache->get_db_for_archive(name);
    }
    if (contents->cache_db != nullptr) {
        if (files.empty()) {
            if (contents->cache_id > 0) {
                try {
                    contents->cache_db->delete_archive(contents->cache_id);
                }
                catch (Exception &exception) {
                    contents->cache_db->seterr();
                    output.error_database("%s: error deleting from %s", name.c_str(), CkmameDB::db_name.c_str());
                    /* TODO: handle errors */
                }
            }
            contents->cache_id = 0;
        }
        else {
            get_last_update();
            
            try {
                contents->cache_db->write_archive(contents.get());
            }
            catch (Exception &exception) {
                contents->cache_db->seterr();
                output.error_database("%s: error writing to %s", name.c_str(), CkmameDB::db_name.c_str());
                contents->cache_id = 0;
            }
        }
    }
    else {
        contents->cache_id = 0;
    }

    cache_changed = false;
}


bool Archive::file_add_empty(const std::string &filename) {
    if (!is_writable()) {
	output.set_error_file(name);
	output.archive_error("cannot add to read-only archive");
	return false;
    }

    Hashes hashes;
    hashes.size = 0;
    hashes.add_types(Hashes::TYPE_ALL);
    Hashes::Update hu(&hashes);
    hu.end();

    add_file(filename, &hashes, nullptr);
    changes[files.size() - 1].source = std::make_shared<ZipSource>(zip_source_buffer_create(nullptr, 0, 0, nullptr));

    return true;
}


bool Archive::file_copy(Archive *source_archive, uint64_t source_index, const std::string &filename) {
    return file_copy_part(source_archive, source_index, filename, 0, {}, &source_archive->files[source_index].hashes);
}

bool Archive::file_copy_or_move(Archive *source_archive, uint64_t source_index, const std::string &filename, bool copy) {
    if (copy) {
        return file_copy(source_archive, source_index, filename);
    }
    else {
        return file_move(source_archive, source_index, filename);
    }
}


bool Archive::file_copy_part(Archive *source_archive, uint64_t source_index, const std::string &filename, uint64_t start, std::optional<uint64_t> length, const Hashes *hashes) {
    if (!is_writable()) {
        output.set_error_file(name);
	output.archive_error("cannot add to read-only archive");
	return false;
    }

    if (file_index_by_name(filename).has_value()) {
        output.set_error_file(name);
	errno = EEXIST;
	output.archive_error("can't copy to %s: %s", filename.c_str(), strerror(errno));
	return false;
    }
    output.set_error_archive(source_archive->files[source_index].name, name);
    if (source_archive->files[source_index].broken) {
	output.archive_file_error("not copying broken file");
	return false;
    }
    if (source_archive->changes[source_index].status == Change::ADDED) {
	output.archive_error("cannot copy added file");
	return false;
    }
    if (length.has_value()) {
        if (start + length.value() > source_archive->files[source_index].hashes.size) {
            output.archive_error("invalid range (%" PRIu64 ", %" PRIu64 ")", start, length.value());
            return false;
        }
    }
    else {
        if (start > source_archive->files[source_index].hashes.size) {
            output.archive_error("invalid start offset %" PRIu64, start);
            return false;
        }
    }

    bool full_file = start == 0 && (!length.has_value() || length.value() == source_archive->files[source_index].hashes.size);
    
    if (full_file) {
        add_file(filename, &source_archive->files[source_index].hashes, &source_archive->files[source_index].detector_hashes);
    }
    else {
        add_file(filename, hashes, nullptr);
    }

    if (have_direct_file_access() && source_archive->have_direct_file_access() && full_file) {
        changes[files.size() - 1].file = source_archive->get_original_filename(source_index);
    }
    else {
        try {
            changes[files.size() - 1].source = source_archive->get_source(source_index, start, length);
        }
        catch (Exception &ex) {
            files.pop_back();
            changes.pop_back();
            return false;
        }
    }

    return true;
}


bool Archive::file_delete(uint64_t index) {
    if (!is_writable()) {
	output.set_error_file(name);
	output.archive_error("cannot delete from read-only archive");
	return false;
    }

    if (changes[index].status != Change::EXISTS) {
	output.set_error_file(name);
	output.archive_error("cannot delete broken/added/deleted file");
	return false;
    }

    changes[index].status = Change::DELETED;
    modified = true;

    return true;
}


bool Archive::file_move(Archive *source_archive, uint64_t source_index, const std::string &filename) {
    if (!file_copy(source_archive, source_index, filename)) {
        return false;
    }

    return source_archive->file_delete(source_index);
}

bool Archive::file_rename(uint64_t index, const std::string &filename) {
    output.set_error_file(name);

    if (!is_writable()) {
	output.archive_error("cannot rename in read-only archive");
	return false;
    }
    if (changes[index].status != Change::EXISTS) {
	output.archive_error("cannot copy broken/added/deleted file");
	return false;
    }

    if (file_index_by_name(filename).has_value()) {
	errno = EEXIST;
	output.archive_error("can't rename %s to %s: %s", files[index].name.c_str(), filename.c_str(), strerror(errno));
	return false;
    }

    if (changes[index].original_name.empty()) {
        changes[index].original_name = files[index].name;
    }
    files[index].name = filename;
    modified = true;

    return true;
}


bool Archive::file_rename_to_unique(uint64_t index) {
    if (!is_writable()) {
	output.set_error_file(name);
	output.archive_error("cannot rename in read-only archive");
	return false;
    }

    auto new_name = make_unique_name_in_archive(files[index].name);
    if (new_name.empty()) {
        return false;
    }

    return file_rename(index, new_name);
}


bool Archive::rollback() {
    if (!modified) {
        return false;
    }

    modified = false;
    
    for (size_t i = 0; i < files.size(); i++) {
        auto &file = files[i];
        auto &change = changes[i];
        
        if (!change.original_name.empty()) {
            file.name = change.original_name;
        }
        change.file = "";
        change.source = nullptr;
        
        switch (change.status) {
            case Change::DELETED:
                change.status = Change::EXISTS;
                break;
                
            case Change::ADDED:
                files.erase(files.begin() + i);
                changes.erase(changes.begin() + i);
                i--;
                break;

            default:
                break;
        }
    }

    return true;
}


void Archive::add_file(const std::string &filename, const Hashes *hashes, const std::unordered_map<size_t, Hashes> *detector_hashes) {
    File file;
    Change change;

    file.hashes = *hashes;
    if (detector_hashes != nullptr) {
        file.detector_hashes = *detector_hashes;
    }
    file.name = filename;
    file.filename_extension = contents->filename_extension;
    change.status = Change::ADDED;

    files.push_back(file);
    changes.push_back(change);
    
    modified = true;
}
