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

#include <errno.h>
#include <stddef.h>

#include "array.h"
#include "dbh_cache.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "sq_util.h"
#include "util.h"
#include "xmalloc.h"

class CacheDirectory {
public:
    std::string name;
    dbh_t *dbh;
    bool initialized;
    
    CacheDirectory(const std::string &name_): name(name_), dbh(NULL), initialized(false) { }
};

static std::vector<CacheDirectory> cache_directories;

static std::string dbh_cache_archive_name(dbh_t *dbh, const std::string &name);
static int dbh_cache_delete_files(dbh_t *, int);
static int dbh_cache_write_archive(dbh_t *dbh, int id, const std::string &name, time_t mtime, size_t size);


bool dbh_cache_close_all(void) {
    auto ok = true;

    for (auto &directory : cache_directories) {
        if (directory.dbh) {
            bool empty = dbh_cache_is_empty(directory.dbh);
            std::string filename = sqlite3_db_filename(dbh_db(directory.dbh), "main");
            
            if (dbh_close(directory.dbh) != 0) {
                ok = false;
            }
	    /* TODO: hack; cache should have detector-applied hashes
	     * or both; currently only has useless ones without
	     * detector applied, which breaks consecutive runs */
	    if (empty || detector) {
                if (remove(filename.c_str()) != 0) {
		    myerror(ERRSTR, "can't remove empty database '%s'", filename.c_str());
                    ok = false;
		}
	    }
	}
	directory.dbh = NULL;
        directory.initialized = false;
    }

    return ok;
}


int
dbh_cache_delete(dbh_t *dbh, int id) {
    sqlite3_stmt *stmt;
    
    if (dbh_cache_delete_files(dbh, id) < 0)
	return -1;

    if ((stmt = dbh_get_statement(dbh, DBH_STMT_DIR_DELETE_ARCHIVE)) == NULL)
	return -1;
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE)
	return -1;

    return 0;
}


int
dbh_cache_delete_by_name(dbh_t *dbh, const char *name) {
    int id = dbh_cache_get_archive_id(dbh, name);

    if (id < 0) {
	return -1;
    }

    return dbh_cache_delete(dbh, id);
}


int
dbh_cache_delete_files(dbh_t *dbh, int id) {
    sqlite3_stmt *stmt;

    if ((stmt = dbh_get_statement(dbh, DBH_STMT_DIR_DELETE_FILE)) == NULL)
	return -1;
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE)
	return -1;

    return 0;
}


int
dbh_cache_get_archive_id(dbh_t *dbh, const char *name) {
    sqlite3_stmt *stmt;
    if ((stmt = dbh_get_statement(dbh, DBH_STMT_DIR_QUERY_ARCHIVE_ID)) == NULL) {
	return 0;
    }

    auto archive_name = dbh_cache_archive_name(dbh, name);
    if (archive_name.empty()) {
	return 0;
    }

    if (sqlite3_bind_text(stmt, 1, archive_name.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
	return 0;
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
	return 0;
    }

    return sqlite3_column_int(stmt, 0);
}


bool
dbh_cache_get_archive_last_change(dbh_t *dbh, int archive_id, time_t *mtime, off_t *size) {
    sqlite3_stmt *stmt;

    if ((stmt = dbh_get_statement(dbh, DBH_STMT_DIR_QUERY_ARCHIVE_LAST_CHANGE)) == NULL || sqlite3_bind_int(stmt, 1, archive_id) != SQLITE_OK) {
	return false;
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
	return false;
    }

    *mtime = sqlite3_column_int64(stmt, 0);
    *size = sqlite3_column_int64(stmt, 1);

    return true;
}


dbh_t *
dbh_cache_get_db_for_archive(const std::string &name) {
    for (auto &directory : cache_directories) {
        if (name.compare(0, directory.name.length(), directory.name) == 0 && (name.length() == directory.name.length() || name[directory.name.length()] == '/')) {
            if (!directory.initialized) {
		directory.initialized = true;
		if ((fix_options & FIX_DO) == 0) {
		    struct stat st;
		    if (stat(directory.name.c_str(), &st) < 0 && errno == ENOENT)
			return NULL; /* we won't write any files, so DB would remain empty */
		}
                if (ensure_dir(directory.name.c_str(), 0) < 0) {
		    return NULL;
                }

                auto dbname = directory.name + '/' + DBH_CACHE_DB_NAME;

		if ((directory.dbh = dbh_open(dbname, DBH_FMT_DIR | DBH_CREATE | DBH_WRITE)) == NULL) {
		    myerror(ERRDB, "can't open rom directory database for '%s'", directory.name.c_str());
		    return NULL;
		}
	    }
            return directory.dbh;
	}
    }

    return NULL;
}


bool
dbh_cache_is_empty(dbh_t *dbh) {
    sqlite3_stmt *stmt;

    if ((stmt = dbh_get_statement(dbh, DBH_STMT_DIR_COUNT_ARCHIVES)) == NULL)
	return false;

    if (sqlite3_step(stmt) != SQLITE_ROW)
	return false;

    return sqlite3_column_int(stmt, 0) == 0;
}


std::vector<std::string> dbh_cache_list_archives(dbh_t *dbh) {
    sqlite3_stmt *stmt;
    std::vector<std::string> archives;
    int ret;

    if ((stmt = dbh_get_statement(dbh, DBH_STMT_DIR_LIST_ARCHIVES)) == NULL) {
	return archives;
    }

    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        archives.push_back(sq3_get_string(stmt, 0));
    }

    if (ret != SQLITE_DONE) {
        archives.clear();
    }

    return archives;
}


int
dbh_cache_read(dbh_t *dbh, const std::string &name, std::vector<File> *files) {
    sqlite3_stmt *stmt;
    int ret;
    int archive_id;

    if ((archive_id = dbh_cache_get_archive_id(dbh, name.c_str())) == 0) {
	return 0;
    }

    if ((stmt = dbh_get_statement(dbh, DBH_STMT_DIR_QUERY_FILE)) == NULL) {
	return -1;
    }
    if (sqlite3_bind_int(stmt, 1, archive_id) != SQLITE_OK) {
	return -1;
    }

    files->clear();

    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        File file;

        file.name = sq3_get_string(stmt, 0);
        file.mtime = sqlite3_column_int(stmt, 1);
	file.status = static_cast<status_t>(sqlite3_column_int(stmt, 2));
	file.size = sq3_get_int64_default(stmt, 3, SIZE_UNKNOWN);
	sq3_get_hashes(&file.hashes, stmt, 4);

        files->push_back(file);

    }

    if (ret != SQLITE_DONE) {
        files->clear();
	return -1;
    }

    return archive_id;
}


int
dbh_cache_register_cache_directory(const std::string &directory_name) {
    std::string name;
    
    if (directory_name.empty()) {
        errno = EINVAL;
        myerror(ERRDEF, "directory_name can't be empty");
        return -1;
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
		return -1;
	    }
	    return 0;
	}
    }

    cache_directories.push_back(CacheDirectory(name));

    return 0;
}


int
dbh_cache_write(dbh_t *dbh, int id, const Archive *a) {
    sqlite3_stmt *stmt;

    if (id == 0) {
        id = dbh_cache_get_archive_id(dbh, a->name.c_str());
    }

    if (id != 0) {
	if (dbh_cache_delete(dbh, id) < 0) {
	    return -1;
	}
    }

    auto name = dbh_cache_archive_name(dbh, a->name.c_str());
    if (name.empty()) {
	return -1;
    }

    if ((id = dbh_cache_write_archive(dbh, id, name, a->mtime, a->size)) < 0) {
	return -1;
    }

    if ((stmt = dbh_get_statement(dbh, DBH_STMT_DIR_INSERT_FILE)) == NULL) {
	return -1;
    }

    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK) {
	return -1;
    }

    for (size_t i = 0; i < a->files.size(); i++) {
	const File *f = &a->files[i];
	if (sqlite3_bind_int(stmt, 2, i) != SQLITE_OK || sq3_set_string(stmt, 3, f->name.c_str()) != SQLITE_OK || sqlite3_bind_int64(stmt, 4, f->mtime) != SQLITE_OK || sqlite3_bind_int(stmt, 5, f->status) != SQLITE_OK || sq3_set_int64_default(stmt, 6, f->size, SIZE_UNKNOWN) != SQLITE_OK || sq3_set_hashes(stmt, 7, &f->hashes, 1) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE || sqlite3_reset(stmt) != SQLITE_OK) {
	    return -1;
	}
    }

    return id;
}


static std::string
dbh_cache_archive_name(dbh_t *dbh, const std::string &name) {
    for (auto &directory : cache_directories) {
        if (directory.dbh == dbh) {
            if (name == directory.name) {
                return ".";
            }

            size_t strip_end = 0;

            if (!roms_unzipped) {
                if (name.length() >= 4 && name.compare(name.length() - 4, 4, ".zip") == 0) {
                    strip_end = 4;
                }
            }
            
            auto offset = directory.name.length() + 1;
            auto length = name.length() - offset - strip_end;

            return name.substr(offset, length);
	}
    }

    return "";
}


static int
dbh_cache_write_archive(dbh_t *dbh, int id, const std::string &name, time_t mtime, size_t size) {
    sqlite3_stmt *stmt;

    stmt = dbh_get_statement(dbh, DBH_STMT_DIR_INSERT_ARCHIVE_ID);

    if (stmt == NULL || sq3_set_string(stmt, 1, name) != SQLITE_OK || sqlite3_bind_int64(stmt, 3, mtime) != SQLITE_OK || sqlite3_bind_int64(stmt, 4, size) != SQLITE_OK) {
	return -1;
    }

    if (id > 0) {
	if (sqlite3_bind_int(stmt, 2, id) != SQLITE_OK)
	    return -1;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE)
	return -1;

    if (id <= 0)
	id = (int)sqlite3_last_insert_rowid(dbh_db(dbh)); /* TODO: use int64_t as id */

    return id;
}
