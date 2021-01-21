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

#include <errno.h>

#include "archive.h"
#include "dbh_cache.h"
#include "error.h"
#include "globals.h"
#include "memdb.h"
#include "util.h"


bool Archive::commit() {
    if (modified) {
        seterrinfo("", name);

        cache_changed = true;

        if (!commit_xxx()) {
            return false;
	}

        for (size_t i = 0; i < files.size(); i++) {
	    switch (files[i].where) {
                case FILE_DELETED:
                    if (is_indexed()) {
                        /* TODO: handle error (how?) */
                        memdb_file_delete(contents.get(), i, is_writable());
                    }

                    if (is_writable()) {
                        files.erase(files.begin() + i);
                        i--;
                    }
		break;

                case FILE_ADDED:
                    if (is_indexed()) {
                        /* TODO: handle error (how?) */
                        memdb_file_insert(NULL, contents.get(), i);
                    }
                    files[i].where = FILE_INGAME;
                    break;

                default:
                    break;
	    }
	}

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
	seterrinfo(name, "");
	myerror(ERRZIP, "cannot add to read-only archive");
	return false;
    }

    File file;
    file.size = 0;
    file.hashes.types = Hashes::TYPE_ALL;
    Hashes::Update hu(&file.hashes);
    hu.end();

    add_file(filename, &file);

    if (!file_add_empty_xxx(filename)) {
        files.pop_back();
        return false;
    }
        
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
        seterrinfo(name, "");
	myerror(ERRZIP, "cannot add to read-only archive");
	return false;
    }

    if (filetype != source_archive->filetype) {
        seterrinfo(name, "");
	myerror(ERRZIP, "cannot copy to archive of different type '%s'", name.c_str()); // TODO: filetype name, not archive name
	return false;
    }
    if (file_index_by_name(filename).has_value()) {
        seterrinfo(name, "");
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
	myerror(ERRZIP, "cannot copy broken/added file");
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

    if (start == 0 && (!length.has_value() || length.value() == source_archive->files[source_index].size)) {
        add_file(filename, &source_archive->files[source_index]);
    }
    else {
        add_file(filename, f);
    }

    if (is_writable()) {
        if (!file_copy_xxx(std::optional<uint64_t>(), source_archive, source_index, filename, start, length)) {
            files.erase(files.end() - 1);
	    return false;
	}
    }

    return true;
}


bool Archive::file_delete(uint64_t index) {
    if (!is_writable()) {
	seterrinfo(name, "");
	myerror(ERRZIP, "cannot delete from read-only archive");
	return false;
    }

    if (files[index].where != FILE_INGAME) {
	seterrinfo(name, "");
	myerror(ERRZIP, "cannot delete broken/added/deleted file");
	return false;
    }

    if (!file_delete_xxx(index)) {
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
    seterrinfo(name, "");

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

    if (!file_rename_xxx(index, filename)) {
        return false;
    }
    
    files[index].name = filename;
    modified = true;

    return true;
}


bool Archive::file_rename_to_unique(uint64_t index) {
    if (!is_writable()) {
	seterrinfo(name, "");
	myerror(ERRZIP, "cannot rename in read-only archive");
	return false;
    }

    auto new_name = make_unique_name(files[index].name);
    if (new_name.empty()) {
        return false;
    }

    return file_rename(index, new_name);
}


bool Archive::rollback() {
    if (!modified) {
        return false;
    }

    if (!rollback_xxx()) {
        return false;
    }

    modified = false;
    
    for (uint64_t i = 0; i < files.size(); i++) {
        auto &file = files[i];
        
        switch (file.where) {
            case FILE_DELETED:
                file.where = FILE_INGAME;
                break;
                
            case FILE_ADDED:
                files.erase(files.begin() + i);
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
    nf = &files[files.size() - 1];

    nf->name = filename;
    nf->where = FILE_ADDED;

    modified = true;
}
