/*
  DB.cc -- object layer around SQLite3
  Copyright (C) 1999-2021 Dieter Baron and Thomas Klausner

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
#include "types.h"

const int StatementID::have_size = 0x10000;
const int StatementID::parameterized = 0x20000;

static const char *format_name[] = {
    "mame.db",
    "in-memory",
    ".ckmame.db"
};

#define USER_VERSION_MAGIC 17000
#define USER_VERSION(format, version)  ((version) + ((format) << 8) + USER_VERSION_MAGIC)
#define USER_VERSION_VALID(user_version) ((user_version) >= USER_VERSION_MAGIC)
#define USER_VERSION_FORMAT(user_version) (((user_version) - USER_VERSION_MAGIC) >> 8)
#define USER_VERSION_VERSION(user_version) (((user_version) - USER_VERSION_MAGIC) & 0xff)

#define SET_VERSION_FMT "pragma user_version = %d"

#define PRAGMAS "PRAGMA synchronous = OFF; "

const std::unordered_map<MigrationVersions, std::string> DB::no_migrations = { };

int DB::get_version(const DBFormat &format) {
    auto stmt = DBStatement(db, "pragma user_version");
    
    if (!stmt.step()) {
        throw Exception("can't get version: %s", sqlite3_errmsg(db));
    }
    
    auto db_user_version = stmt.get_int("user_version");

    if (!USER_VERSION_VALID(db_user_version)) {
        throw Exception("not a ckmame db");
    }

    auto db_format = USER_VERSION_FORMAT(db_user_version);
    auto db_version = USER_VERSION_VERSION(db_user_version);
    
    if (db_format != format.id) {
        if (db_format >= 0 && static_cast<size_t>(db_format) < sizeof(format_name) / sizeof(format_name[0])) {
            throw Exception("invalid db format '%s', expected '%s'", format_name[db_format], format_name[format.id]);
        }
        else {
            throw Exception("invalid db format %d, expected '%s'", db_format, format_name[format.id]);
        }
    }
    
    return db_version;
}


void DB::check_version(const DBFormat &format) {
    auto db_version = get_version(format);
        
    if (db_version == format.version) {
        return;
    }
    
    if (db_version > format.version) {
        throw Exception("database version too new: %d, expected %d", db_version, format.version);
    }
    
    migrate(format, db_version, format.version);
}

void DB::migrate(const DBFormat &format, int from_version, int to_version) {
    std::vector<MigrationStep> migration_steps;
    
    while (from_version < to_version) {
        bool made_progress = false;
        for (auto next_version = to_version; next_version > from_version; next_version -= 1) {
            auto it = format.migrations.find(MigrationVersions(from_version, next_version));
            
            if (it != format.migrations.end()) {
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
        upgrade(format.id, step.version, step.statement);
    }
}


DB::~DB() {
    close();
}

void DB::close() {
    statements.clear();

    if (db) {
        sqlite3_close(db);
    }
}


std::string DB::error() {
    if (db == nullptr) {
	return strerror(ENOMEM);
    }
    return sqlite3_errmsg(db);
}


DB::DB(const DB::DBFormat &format, const std::string &name, int mode) : db(nullptr) {
    auto needs_init = false;
    
    if (mode & DBH_TRUNCATE) {
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
    
    if (mode & DBH_WRITE) {
	sql3_flags = SQLITE_OPEN_READWRITE;
    }
    else {
	sql3_flags = SQLITE_OPEN_READONLY;
    }

    if (mode & DBH_CREATE) {
	sql3_flags |= SQLITE_OPEN_CREATE;
        if (name[0] == ':' || !std::filesystem::exists(name)) {
	    needs_init = true;
	}
    }
    
    try {
        open(format, name, sql3_flags, needs_init);
    }
    catch (Exception &e) {
        close();
        throw e;
    }
}


void DB::open(const DBFormat &format, const std::string &name, int sql3_flags, bool needs_init) {
    if (sqlite3_open_v2(name.c_str(), &db, sql3_flags, nullptr) != SQLITE_OK) {
        throw Exception("%s", sqlite3_errmsg(db));
    }

    if (sqlite3_exec(db, PRAGMAS, nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw Exception("can't set options: %s", sqlite3_errmsg(db));
    }
        
    if (needs_init) {
        upgrade(format.id, format.version, format.init_sql);
    }
    else {
        check_version(format);
    }
}


void DB::upgrade(int format, int version, const std::string &statement) {
    upgrade(db, format, version, statement);
}


void DB::upgrade(sqlite3 *db, int format, int version, const std::string &statement) {
    if (sqlite3_exec(db, "begin exclusive transaction", nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw Exception("can't begin transaction");
    }
    if (sqlite3_exec(db, statement.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        auto error = sqlite3_errmsg(db);
        sqlite3_exec(db, "rollback transaction", nullptr, nullptr, nullptr);
        throw Exception("can't set schema: %s", error);
    }
    
    char b[256];
    sprintf(b, SET_VERSION_FMT, USER_VERSION(format, version));
    if (sqlite3_exec(db, b, nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "rollback transaction", nullptr, nullptr, nullptr);
        throw Exception("can't set version: %s", sqlite3_errmsg(db));
    }
    
    if (sqlite3_exec(db, "commit transaction", nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "rollback transaction", nullptr, nullptr, nullptr);
        throw Exception("can't commit schema: %s", sqlite3_errmsg(db));
    }
}


DBStatement *DB::get_statement_internal(int name) {
    return get_statement_internal(StatementID(name));
}

DBStatement *DB::get_statement_internal(int name, const Hashes &hashes, bool have_size) {
    return get_statement_internal(StatementID(name, hashes, have_size));
}
 
DBStatement *DB::get_statement_internal(StatementID statement_id) {
    auto it = statements.find(statement_id);
    
    if (it != statements.end()) {
        it->second->reset();
        return it->second.get();
    }
    
    auto sql_query = get_query(statement_id.name, statement_id.is_parameterized());
    
    if (sql_query.empty()) {
        throw Exception("invalid statement id " + std::to_string(statement_id.name));
    }
    
    if (statement_id.is_parameterized()) {
        // printf("#DEBUG exanding %x '%s' to ", statement_id.flags, sql_query.c_str());
        auto start = sql_query.find("@SIZE@");
        if (start != std::string::npos) {
            sql_query.replace(start, 6, statement_id.has_size() ? "and size = :size" : "");
        }
        start = sql_query.find("@HASH@");
        if (start != std::string::npos) {
            std::string expanded;
            for (auto i = 1; i <= Hashes::TYPE_MAX; i <<= 1) {
                if (statement_id.has_hash(i)) {
                    auto name = Hashes::type_name(i);
                    expanded += " and (f." + name + " is :" + name + " or f." + name + " is null)";
                }
            }
            
            sql_query.replace(start, 6, expanded);
        }
        // printf(" '%s'\n", sql_query.c_str());
    }

    auto stmt = std::make_shared<DBStatement>(db, sql_query);
    statements[statement_id] = stmt;

    return stmt.get();
}
