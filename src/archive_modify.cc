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

#include "dbh_cache.h"
#include "error.h"
#include "Exception.h"
#include "MemDB.h"

bool Archive::commit() {
    if (modified) {
        seterrinfo("", name);

        cache_changed = true;

        if (!commit_xxx()) {
            return false;
	}

        for (size_t index = 0; index < files.size(); index++) {
            auto &file = files[index];

            switch (file.where) {
                case FILE_DELETED:
                    if (is_indexed()) {
                        /* TODO: handle error (how?) */
                        MemDB::delete_file(contents.get(), index, is_writable());
                    }

                    if (is_writable()) {
                        files.erase(files.begin() + static_cast<ssize_t>(index));
                        index--;
                    }
		break;

                case FILE_ADDED:
                    if (is_indexed()) {
                        /* TODO: handle error (how?) */
                        MemDB::insert_file(NULL, contents.get(), index);
                    }
                    file.where = FILE_INGAME;
                    break;

                default:
                    break;
	    }
	}
        
        contents->changes.clear();
        contents->changes.resize(files.size());

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
    
    if (contents->cache_db == NULL) {
        contents->cache_db = dbh_cache_get_db_for_archive(name);
    }
    if (contents->cache_db != NULL) {
        if (files.empty()) {
            if (contents->cache_id > 0) {
                if (dbh_cache_delete(contents->cache_db.get(), contents->cache_id) < 0) {
                    seterrdb(contents->cache_db.get());
                    myerror(ERRDB, "%s: error deleting from " DBH_CACHE_DB_NAME, name.c_str());
                    /* TODO: handle errors */
                }
            }
            contents->cache_id = 0;
        }
        else {
            get_last_update();
            
            contents->cache_id = dbh_cache_write(contents->cache_db.get(), contents->cache_id, contents.get());
            if (contents->cache_id < 0) {
                seterrdb(contents->cache_db.get());
                myerror(ERRDB, "%s: error writing to " DBH_CACHE_DB_NAME, name.c_str());
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
	seterrinfo(name);
	myerror(ERRZIP, "cannot add to read-only archive");
	return false;
    }

    File file;
    file.size = 0;
    file.hashes.types = Hashes::TYPE_ALL;
    Hashes::Update hu(&file.hashes);
    hu.end();

    add_file(filename, &file);
    contents->changes[files.size() - 1].source = std::make_shared<ZipSource>(zip_source_buffer_create(NULL, 0, 0, NULL));

    return true;
}


bool Archive::file_copy(Archive *source_archive, uint64_t source_index, const std::string &filename) {
    return file_copy_part(source_archive, source_index, filename, 0, {}, &source_archive->files[source_index]);
}

bool Archive::file_copy_or_move(Archive *source_archive, uint64_t source_index, const std::string &filename, bool copy) {
    if (copy) {
        return file_copy(source_archive, source_index, filename);
    }
    else {
        return file_move(source_archive, source_index, filename);
    }
}


bool Archive::file_copy_part(Archive *source_archive, uint64_t source_index, const std::string &filename, uint64_t start, std::optional<uint64_t> length, const File *f) {
    if (!is_writable()) {
        seterrinfo(name);
	myerror(ERRZIP, "cannot add to read-only archive");
	return false;
    }

#if 0
    if (filetype != source_archive->filetype) {
        seterrinfo(name);
	myerror(ERRZIP, "cannot copy to archive of different type '%s'", name.c_str()); // TODO: filetype name, not archive name
	return false;
    }
#endif
    
    if (file_index_by_name(filename).has_value()) {
        seterrinfo(name);
	errno = EEXIST;
	myerror(ERRZIP, "can't copy to %s: %s", filename.c_str(), strerror(errno));
	return false;
    }
    seterrinfo(name, source_archive->files[source_index].name);
    if (source_archive->files[source_index].status == STATUS_BADDUMP) {
	myerror(ERRZIPFILE, "not copying broken file");
	return false;
    }
    if (source_archive->files[source_index].where != FILE_INGAME && source_archive->files[source_index].where != FILE_DELETED) {
	myerror(ERRZIP, "cannot copy added/deleted file");
	return false;
    }
    if (length.has_value()) {
        if (start + length.value() > source_archive->files[source_index].size) {
            myerror(ERRZIP, "invalid range (%" PRIu64 ", %" PRIu64 ")", start, length.value());
            return false;
        }
    }
    else {
        if (start > source_archive->files[source_index].size) {
            myerror(ERRZIP, "invalid start offset %" PRIu64, start);
            return false;
        }
    }

    bool full_file = start == 0 && (!length.has_value() || length.value() == source_archive->files[source_index].size);
    
    if (full_file) {
        add_file(filename, &source_archive->files[source_index]);
    }
    else {
        add_file(filename, f);
    }

    if (have_direct_file_access() && source_archive->have_direct_file_access() && full_file) {
        contents->changes[files.size() - 1].file = source_archive->get_original_filename(source_index);
    }
    else {
        try {
            contents->changes[files.size() - 1].source = source_archive->get_source(source_index, start, length);
        }
        catch (Exception ex) {
            files.pop_back();
            contents->changes.pop_back();
            return false;
        }
    }

    return true;
}


bool Archive::file_delete(uint64_t index) {
    if (!is_writable()) {
	seterrinfo(name);
	myerror(ERRZIP, "cannot delete from read-only archive");
	return false;
    }

    if (files[index].where != FILE_INGAME) {
	seterrinfo(name);
	myerror(ERRZIP, "cannot delete broken/added/deleted file");
	return false;
    }

    files[index].where = FILE_DELETED;
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
    seterrinfo(name);

    if (!is_writable()) {
	myerror(ERRZIP, "cannot rename in read-only archive");
	return false;
    }
    if (files[index].where != FILE_INGAME) {
	myerror(ERRZIP, "cannot copy broken/added/deleted file");
	return false;
    }

    if (file_index_by_name(filename).has_value()) {
	errno = EEXIST;
	myerror(ERRZIP, "can't rename %s to %s: %s", files[index].name.c_str(), filename.c_str(), strerror(errno));
	return false;
    }

    if (contents->changes[index].original_name.empty()) {
        contents->changes[index].original_name = files[index].name;
    }
    files[index].name = filename;
    modified = true;

    return true;
}


bool Archive::file_rename_to_unique(uint64_t index) {
    if (!is_writable()) {
	seterrinfo(name);
	myerror(ERRZIP, "cannot rename in read-only archive");
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
        auto &change = contents->changes[i];
        
        if (!change.original_name.empty()) {
            file.name = change.original_name;
        }
        change.file = "";
        change.source = nullptr;
        
        switch (file.where) {
            case FILE_DELETED:
                file.where = FILE_INGAME;
                break;
                
            case FILE_ADDED:
                files.erase(files.begin() + static_cast<ssize_t>(i));
                contents->changes.erase(contents->changes.begin() + static_cast<ssize_t>(i));
                i--;
                break;

            default:
                break;
        }
    }

    return true;
}


void Archive::add_file(const std::string &filename, const File *file) {
    File *nf;

    files.push_back(*file);
    contents->changes.push_back(ArchiveContents::Changes());
    
    nf = &files[files.size() - 1];

    nf->name = filename;
    nf->where = FILE_ADDED;
    nf->filename_extension = contents->filename_extension;

    modified = true;
}
