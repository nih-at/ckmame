/*
 dbh_cache.c -- files in dirs sqlite3 data base
 Copyright (C) 2014-2015 Dieter Baron and Thomas Klausner

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

#include "CkmameDB.h"

#include <filesystem>

#include "Detector.h"
#include "error.h"
#include "Exception.h"
#include "fix.h"
#include "sq_util.h"
#include "util.h"


std::vector<CkmameDB::CacheDirectory> CkmameDB::cache_directories;


CkmameDB::CkmameDB(const std::string &dbname, const std::string &directory_) : db(dbname, DBH_FMT_DIR | DBH_CREATE | DBH_WRITE), directory(directory_) {
    sqlite3_stmt *stmt;
    if ((stmt = db.get_statement(DBH_STMT_DIR_LIST_DETECTORS)) == NULL) {
        throw Exception();
    }

    int ret;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        detector_ids.add(DetectorDescriptor(sq3_get_string(stmt, 1), sq3_get_string(stmt, 2)), sq3_get_uint64(stmt, 0));
    }

    if (ret != SQLITE_DONE) {
        throw Exception();
    }
}


bool CkmameDB::close_all() {
    auto ok = true;

    for (auto &directory : cache_directories) {
        if (directory.db) {
            bool empty = directory.db->is_empty();
            std::string filename = sqlite3_db_filename(directory.db->db.db, "main");
            
            directory.db = NULL;
            if (empty) {
                std::error_code ec;
                std::filesystem::remove(filename);
                if (ec) {
                    myerror(ERRDEF, "can't remove empty database '%s': %s", filename.c_str(), ec.message().c_str());
                    ok = false;
                }
            }
        }
        directory.initialized = false;
    }
    
    return ok;
}


void CkmameDB::delete_archive(int id) {
    sqlite3_stmt *stmt;
    
    delete_files(id);

    if ((stmt = db.get_statement(DBH_STMT_DIR_DELETE_ARCHIVE)) == NULL
        || sqlite3_bind_int(stmt, 1, id) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE) {
        throw Exception();
    }
}


void CkmameDB::delete_archive(const std::string &name, filetype_t filetype) {
    auto id = get_archive_id(name, filetype);

    if (id < 0) {
        throw Exception();
    }

    delete_archive(id);
}


void CkmameDB::delete_files(int id) {
    sqlite3_stmt *stmt;

    if ((stmt = db.get_statement(DBH_STMT_DIR_DELETE_FILE)) == NULL
        || sqlite3_bind_int(stmt, 1, id) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE) {
        throw Exception();
    }
}


int CkmameDB::get_archive_id(const std::string &name, filetype_t filetype) {
    sqlite3_stmt *stmt;
    if ((stmt = db.get_statement(DBH_STMT_DIR_QUERY_ARCHIVE_ID)) == NULL) {
	return 0;
    }

    auto archive_name = name_in_db(name);
    if (archive_name.empty()) {
	return 0;
    }

    if (sqlite3_bind_text(stmt, 1, archive_name.c_str(), -1, SQLITE_STATIC) != SQLITE_OK
        || sqlite3_bind_int(stmt, 2, filetype) != SQLITE_OK
        || sqlite3_step(stmt) != SQLITE_ROW) {
	return 0;
    }

    return sqlite3_column_int(stmt, 0);
}


void CkmameDB::get_last_change(int id, time_t *mtime, off_t *size) {
    sqlite3_stmt *stmt;

    if ((stmt = db.get_statement(DBH_STMT_DIR_QUERY_ARCHIVE_LAST_CHANGE)) == NULL
        || sqlite3_bind_int(stmt, 1, id) != SQLITE_OK
        || sqlite3_step(stmt) != SQLITE_ROW) {
        throw Exception();
    }

    *mtime = sqlite3_column_int64(stmt, 0);
    *size = sqlite3_column_int64(stmt, 1);
}


CkmameDBPtr CkmameDB::get_db_for_archvie(const std::string &name) {
    for (auto &directory : cache_directories) {
        if (name.compare(0, directory.name.length(), directory.name) == 0 && (name.length() == directory.name.length() || name[directory.name.length()] == '/')) {
            if (!directory.initialized) {
		directory.initialized = true;
		if ((fix_options & FIX_DO) == 0) {
		    std::error_code ec;
		    if (!std::filesystem::exists(directory.name, ec)) {
			return NULL; /* we won't write any files, so DB would remain empty */
		    }
		}
                if (!ensure_dir(directory.name, false)) {
		    return NULL;
                }

                auto dbname = directory.name + '/' + DBH_CACHE_DB_NAME;

                try {
                    directory.db = std::make_shared<CkmameDB>(dbname, directory.name);
                }
                catch (std::exception &e) {
                    myerror(ERRDB, "can't open rom directory database for '%s': %s", directory.name.c_str(), e.what());
		    return NULL;
		}
	    }
            return directory.db;
	}
    }

    return NULL;
}


bool CkmameDB::is_empty() {
    sqlite3_stmt *stmt;

    if ((stmt = db.get_statement(DBH_STMT_DIR_COUNT_ARCHIVES)) == NULL
        || sqlite3_step(stmt) != SQLITE_ROW) {
	return false;
    }

    return sqlite3_column_int(stmt, 0) == 0;
}


std::vector<ArchiveLocation> CkmameDB::list_archives() {
    sqlite3_stmt *stmt;
    std::vector<ArchiveLocation> archives;
    int ret;

    if ((stmt = db.get_statement(DBH_STMT_DIR_LIST_ARCHIVES)) == NULL) {
	return archives;
    }

    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        archives.push_back(ArchiveLocation(sq3_get_string(stmt, 0), static_cast<filetype_t>(sqlite3_column_int(stmt, 1))));
    }

    if (ret != SQLITE_DONE) {
        archives.clear();
    }

    return archives;
}


int CkmameDB::read_files(int archive_id, std::vector<File> *files) {
    if (archive_id == 0) {
	return 0;
    }

    sqlite3_stmt *stmt;
    if ((stmt = db.get_statement(DBH_STMT_DIR_QUERY_FILE)) == NULL
        || sqlite3_bind_int(stmt, 1, archive_id) != SQLITE_OK) {
        throw Exception();
    }

    files->clear();

    int ret;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        auto detector_id = sq3_get_uint64(stmt, 1);
        
        if (detector_id == 0) {
            // There is exactly one entry per file_idx with detector_id 0, which is retrieved in order.
            File file;
            
            file.name = sq3_get_string(stmt, 2);
            file.mtime = sqlite3_column_int(stmt, 3);
            file.broken = sqlite3_column_int(stmt, 4) != 0;
            file.hashes.size = sq3_get_uint64_default(stmt, 5, Hashes::SIZE_UNKNOWN);
            sq3_get_hashes(&file.hashes, stmt, 6);
            
            files->push_back(file);
        }
        else {
            auto file_id = sq3_get_uint64(stmt, 0);
            auto global_detector_id = get_global_detector_id(detector_id);
            
            Hashes hashes;
            hashes.size = sq3_get_uint64_default(stmt, 5, Hashes::SIZE_UNKNOWN);
            sq3_get_hashes(&hashes, stmt, 6);
            
            (*files)[file_id].detector_hashes[global_detector_id] = hashes;
        }
    }

    if (ret != SQLITE_DONE) {
        files->clear();
        throw  Exception();
    }

    return archive_id;
}


void CkmameDB::register_directory(const std::string &directory_name) {
    std::string name;
    
    if (directory_name.empty()) {
        errno = EINVAL;
        myerror(ERRDEF, "directory_name can't be empty");
        throw Exception();
    }
    
    if (directory_name[directory_name.length() - 1] == '/') {
        name = directory_name.substr(0, directory_name.length() - 1);
    }
    else {
        name = directory_name;
    }

    for (auto &directory : cache_directories) {
        auto length = std::min(name.length(), directory.name.length());
        
        if (name.compare(0, length, directory.name) == 0 && (name.length() == length || name[length] == '/') && (directory.name.length() == length || directory.name[length] == '/')) {
            if (directory.name.length() != name.length()) {
		myerror(ERRDEF, "can't cache in directory '%s' and its parent '%s'", (directory.name.length() < name.length() ? name.c_str() : directory.name.c_str()), (directory.name.length() < name.length() ? directory.name.c_str() : name.c_str()));
                throw Exception();
	    }
	    return;
	}
    }

    cache_directories.push_back(CacheDirectory(name));

    return;
}


void CkmameDB::seterr() {
    seterrdb(&db);
}


void CkmameDB::write_archive(ArchiveContents *archive) {
    sqlite3_stmt *stmt;

    auto id = archive->cache_id;
    
    if (id == 0) {
        id = get_archive_id(archive->name, archive->filetype);
    }

    if (id != 0) {
        delete_archive(id);
    }

    auto name = name_in_db(archive->name);
    if (name.empty()) {
        throw Exception();
    }

    id = write_archive_header(id, name, archive->filetype, archive->flags & ARCHIVE_FL_TOP_LEVEL_ONLY ? 0 : archive->mtime, archive->size);

    if ((stmt = db.get_statement(DBH_STMT_DIR_INSERT_FILE)) == NULL
        || sqlite3_bind_int(stmt, 1, id) != SQLITE_OK) {
        throw Exception();
    }

    for (size_t i = 0; i < archive->files.size(); i++) {
	const auto *f = &archive->files[i];
	if (sqlite3_bind_int(stmt, 2, static_cast<int>(i)) != SQLITE_OK
            || sq3_set_uint64(stmt, 3, 0) != SQLITE_OK
            || sq3_set_string(stmt, 4, f->name.c_str()) != SQLITE_OK
            || sqlite3_bind_int64(stmt, 5, f->mtime) != SQLITE_OK
            || sqlite3_bind_int(stmt, 6, f->broken ? 1 : 0) != SQLITE_OK
            || sq3_set_uint64(stmt, 7, f->hashes.size) != SQLITE_OK
            || sq3_set_hashes(stmt, 8, &f->hashes, 1) != SQLITE_OK
            || sqlite3_step(stmt) != SQLITE_DONE
            || sqlite3_reset(stmt) != SQLITE_OK) {
            throw Exception();
	}
        
        for (auto &pair : f->detector_hashes) {
            auto id = get_detector_id(pair.first);
            if (sqlite3_bind_int(stmt, 2, static_cast<int>(i)) != SQLITE_OK
                || sq3_set_uint64(stmt, 3, id) != SQLITE_OK
                // file.name can't be NULL, so set it to empty string
                || sqlite3_bind_text(stmt, 4, "", -1, SQLITE_STATIC) != SQLITE_OK
                || sqlite3_bind_int64(stmt, 5, 0) != SQLITE_OK
                || sqlite3_bind_int(stmt, 6, 0) != SQLITE_OK
                || sq3_set_uint64(stmt, 7, pair.second.size) != SQLITE_OK
                || sq3_set_hashes(stmt, 8, &pair.second, 1) != SQLITE_OK
                || sqlite3_step(stmt) != SQLITE_DONE
                || sqlite3_reset(stmt) != SQLITE_OK) {
                    throw Exception();
            }
        }
    }

    archive->cache_id = id;
}


std::string CkmameDB::name_in_db(const std::string &name) {
    if (name == directory) {
        return ".";
    }

    auto offset = directory.length() + 1;
    auto length = name.length() - offset;

    return name.substr(offset, length);
}


int CkmameDB::write_archive_header(int id, const std::string &name, filetype_t filetype, time_t mtime, uint64_t size) {
    auto stmt = db.get_statement(DBH_STMT_DIR_INSERT_ARCHIVE_ID);

    if (stmt == NULL
        || sq3_set_string(stmt, 1, name) != SQLITE_OK
        || sqlite3_bind_int(stmt, 3, filetype) != SQLITE_OK
        || sqlite3_bind_int64(stmt, 4, mtime) != SQLITE_OK
        || sq3_set_uint64(stmt, 5, size) != SQLITE_OK) {
        throw Exception();
    }

    if (id > 0) {
        if (sqlite3_bind_int(stmt, 2, id) != SQLITE_OK) {
            throw Exception();
        }
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw Exception();
    }

    if (id <= 0) {
	id = (int)sqlite3_last_insert_rowid(db.db); /* TODO: use int64_t as id */
    }

    return id;
}


size_t CkmameDB::get_detector_id(size_t global_id) {
    auto detector = Detector::get_descriptor(global_id);
    
    if (detector == NULL) {
        throw Exception();
    }
    
    auto known = detector_ids.known(*detector);
    auto id = detector_ids.get_id(*detector);
    
    if (!known) {
        auto stmt = db.get_statement(DBH_STMT_DIR_INSERT_DETECTOR);
        
        if (stmt == NULL
            || sq3_set_uint64(stmt, 1, id) != SQLITE_OK
            || sq3_set_string(stmt, 2, detector->name) != SQLITE_OK
            || sq3_set_string(stmt, 3, detector->version) != SQLITE_OK
            || sqlite3_step(stmt) != SQLITE_DONE) {
            throw Exception();
        }
    }
    
    return id;
}


size_t CkmameDB::get_global_detector_id(size_t id) {
    auto detector = detector_ids.get_descriptor(id);
    if (detector == NULL) {
        throw Exception();
    }
    return Detector::get_id(*detector);
}
