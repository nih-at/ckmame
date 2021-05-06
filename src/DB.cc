/*
  dbh.c -- mame.db sqlite3 data base
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

#include "DB.h"

#include <cstring>
#include <filesystem>
#include <vector>

#include "Exception.h"

static const int format_version[] = {3, 1, 3};
static const char *format_name[] = {
    "mamedb",
    "in-memory",
    "ckmamedb"
};

#define USER_VERSION_MAGIC 17000
#define USER_VERSION(format, version)  ((version) + ((format) << 8) + USER_VERSION_MAGIC)
#define USER_VERSION_VALID(user_version) ((user_version) >= USER_VERSION_MAGIC)
#define USER_VERSION_FORMAT(user_version) (((user_version) - USER_VERSION_MAGIC) >> 8)
#define USER_VERSION_VERSION(user_version) (((user_version) - USER_VERSION_MAGIC) & 0xff)

#define SET_VERSION_FMT "pragma user_version = %d"

#define PRAGMAS "PRAGMA synchronous = OFF; "

std::unordered_map<DB::MigrationXXX, std::string> DB::migrations = {
    { MigrationXXX(DBH_FMT_DIR, 2, 3), "\
create table detector (\n\
    detector_id integer primary key autoincrement,\n\
    name text not null,\n\
    version text not null\n\
);\n\
create index detector_name_version on detector (name, version);\n\
alter table file add column detector_id integer not null default 0;\n\
create index file_size on file (size);\n\
create index file_crc on file (crc);\n\
create index file_md5 on file (md5);\n\
create index file_sha1 on file (sha1);\n\
    " }
};

int DB::get_version() {
    sqlite3_stmt *stmt;
    
    if ((stmt = get_statement(DBH_STMT_QUERY_VERSION)) == NULL) {
        throw Exception("can't get version: unkown statement");
    }
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        throw Exception("can't get version: %s", sqlite3_errmsg(db));
    }
    
    auto db_user_version = sqlite3_column_int(stmt, 0);

    if (!USER_VERSION_VALID(db_user_version)) {
        throw Exception("not a ckmame db");
    }

    auto db_format = USER_VERSION_FORMAT(db_user_version);
    auto db_version = USER_VERSION_VERSION(db_user_version);
    
    if (db_format != format) {
        if (db_format >= 0 && static_cast<size_t>(db_format) < sizeof(format_name) / sizeof(format_name[0])) {
            throw Exception("invalid db format '%s', expected '%s'", format_name[db_format], format_name[format]);
        }
        else {
            throw Exception("invalid db format %d, expected '%s'", db_format, format_name[format]);
        }
    }
    
    return db_version;
}


void DB::check_version() {
    auto db_version = get_version();
        
    if (db_version == format_version[format]) {
        return;
    }
    
    if (db_version > format_version[format]) {
        throw Exception("database version too new: %d, expected %d", db_version, format_version[format]);
    }
    
    migrate(db_version, format_version[format]);
}

void DB::migrate(int from_version, int to_version) {
    std::vector<MigrationStep> migration_steps;
    
    while (from_version < to_version) {
        bool made_progress = false;
        for (auto next_version = to_version; next_version > from_version; next_version -= 1) {
            auto it = migrations.find(MigrationXXX(format, from_version, next_version));
            
            if (it != migrations.end()) {
                from_version = next_version;
                migration_steps.push_back(MigrationStep(it->second, next_version));
                made_progress = true;
                break;
            }
        }
        
        if (!made_progress) {
            throw Exception("can't migrate from version %d to %d", from_version, to_version);
        }
    }
    
    for (auto &step : migration_steps) {
        upgrade(step.statement, step.version);
    }
}


DB::~DB() {
    close();
}

void DB::close() {
    for (size_t i = 0; i < DBH_STMT_MAX; i++) {
        if (statements[i] != NULL) {
            sqlite3_finalize(statements[i]);
            statements[i] = NULL;
        }
    }

    if (db) {
        sqlite3_close(db);
    }
}


std::string DB::error() {
    if (db == NULL) {
	return strerror(ENOMEM);
    }
    return sqlite3_errmsg(db);
}


DB::DB(const std::string &name, int mode) : db(NULL) {
    format = DBH_FMT(mode);

    if (format < 0 || static_cast<size_t>(format) >= sizeof(format_version) / sizeof(format_version[0])) {
        throw Exception("invalid DB format %d", format);
    }

    for (size_t i = 0; i < DBH_STMT_MAX; i++) {
        statements[i] = NULL;
    }

    auto needs_init = false;
    
    if (DBH_FLAGS(mode) & DBH_TRUNCATE) {
	/* do not delete special cases (like memdb) */
	if (name[0] != ':') {
            std::error_code error;
            std::filesystem::remove(name, error);
            if (error) {
                throw Exception("can't truncate: %s", error.message().c_str());
            }
	}
	needs_init = true;
    }

    int sql3_flags;
    
    if (DBH_FLAGS(mode) & DBH_WRITE) {
	sql3_flags = SQLITE_OPEN_READWRITE;
    }
    else {
	sql3_flags = SQLITE_OPEN_READONLY;
    }

    if (DBH_FLAGS(mode) & DBH_CREATE) {
	sql3_flags |= SQLITE_OPEN_CREATE;
        if (name[0] == ':' || !std::filesystem::exists(name)) {
	    needs_init = true;
	}
    }
    
    try {
        open(name, sql3_flags, needs_init);
    }
    catch (Exception &e) {
        close();
        throw e;
    }
}


void DB::open(const std::string &name, int sql3_flags, bool needs_init) {
    if (sqlite3_open_v2(name.c_str(), &db, sql3_flags, NULL) != SQLITE_OK) {
        throw Exception("%s", sqlite3_errmsg(db));
    }

    if (sqlite3_exec(db, PRAGMAS, NULL, NULL, NULL) != SQLITE_OK) {
        throw Exception("can't set options: %s", sqlite3_errmsg(db));
    }
        
    if (needs_init) {
        init();
    }
    else {
        check_version();
    }
}

void DB::init() {
    upgrade(sql_db_init[format], format_version[format]);
}


void DB::upgrade(const std::string &statement, int version) {
    upgrade(db, format, version, statement);
}


void DB::upgrade(sqlite3 *db, int format, int version, const std::string &statement) {
    if (sqlite3_exec(db, "begin exclusive transaction", NULL, NULL, NULL) != SQLITE_OK) {
        throw Exception("can't begin transaction");
    }
    if (sqlite3_exec(db, statement.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
        sqlite3_exec(db, "rollback transaction", NULL, NULL, NULL);
        throw Exception("can't set schema: %s", sqlite3_errmsg(db));
    }
    
    char b[256];
    sprintf(b, SET_VERSION_FMT, USER_VERSION(format, version));
    if (sqlite3_exec(db, b, NULL, NULL, NULL) != SQLITE_OK) {
        sqlite3_exec(db, "rollback transaction", NULL, NULL, NULL);
        throw Exception("can't set version: %s", sqlite3_errmsg(db));
    }
    
    if (sqlite3_exec(db, "commit transaction", NULL, NULL, NULL) != SQLITE_OK) {
        sqlite3_exec(db, "rollback transaction", NULL, NULL, NULL);
        throw Exception("can't commit schema: %s", sqlite3_errmsg(db));
    }
}


sqlite3_stmt *DB::get_statement(dbh_stmt_t stmt_id, const Hashes *hashes, bool have_size) {
    for (int i = 1; i <= Hashes::TYPE_MAX; i <<= 1) {
        if (hashes->has_type(i)) {
            stmt_id = static_cast<dbh_stmt_t>(stmt_id + i);
        }
    }
    if (have_size) {
        stmt_id = static_cast<dbh_stmt_t>(stmt_id + (Hashes::TYPE_MAX << 1));
    }

    return get_statement(stmt_id);
}


sqlite3_stmt *DB::get_statement(dbh_stmt_t stmt_id) {
    if (stmt_id >= DBH_STMT_MAX) {
	return NULL;
    }

    if (statements[stmt_id] == NULL) {
        if (sqlite3_prepare_v2(db, dbh_stmt_sql[stmt_id], -1, &(statements[stmt_id]), NULL) != SQLITE_OK) {
            statements[stmt_id] = NULL;
	    return NULL;
	}
    }
    else {
        if (sqlite3_reset(statements[stmt_id]) != SQLITE_OK || sqlite3_clear_bindings(statements[stmt_id]) != SQLITE_OK) {
            sqlite3_finalize(statements[stmt_id]);
            statements[stmt_id] = NULL;
            return NULL;
	}
    }
    
    return statements[stmt_id];
}
