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

#include "dbh.h"

#include <cstring>
#include <filesystem>

static const int format_version[] = {3, 1, 2};
#define USER_VERSION(fmt) (format_version[fmt] + (fmt << 8) + 17000)

#define SET_VERSION_FMT "pragma user_version = %d"

#define PRAGMAS "PRAGMA synchronous = OFF; "


bool DB::check_version() {
    sqlite3_stmt *stmt;

    if ((stmt = get_statement(DBH_STMT_QUERY_VERSION)) == NULL) {
	/* TODO */
	return false;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW) {
	/* TODO */
	return false;
    }

    version_ok = (sqlite3_column_int(stmt, 0) == USER_VERSION(format));

    return version_ok;
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

    if (!version_ok) {
        return "Database format version mismatch";
    }
    else {
        return sqlite3_errmsg(db);
    }
}


DB::DB(const std::string &name, int mode) : db(NULL) {
    if (DBH_FMT(mode) > sizeof(format_version) / sizeof(format_version[0])) {
	errno = EINVAL;
        throw std::exception();
    }

    for (size_t i = 0; i < DBH_STMT_MAX; i++) {
        statements[i] = NULL;
    }

    format = DBH_FMT(mode);

    auto needs_init = false;
    
    if (DBH_FLAGS(mode) & DBH_TRUNCATE) {
	/* do not delete special cases (like memdb) */
	if (name[0] != ':') {
            std::filesystem::remove(name);
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
    
    if (!open(name, sql3_flags, needs_init)) {
        auto save = errno;
        close();
        errno = save;
        throw std::exception();
    }
}


bool DB::open(const std::string &name, int sql3_flags, bool needs_init) {
    if (sqlite3_open_v2(name.c_str(), &db, sql3_flags, NULL) != SQLITE_OK) {
        return false;
    }

    if (sqlite3_exec(db, PRAGMAS, NULL, NULL, NULL) != SQLITE_OK) {
        return false;
    }
        
    if (needs_init) {
        if (!init()) {
            return false;
        }
    }
    else if (!check_version()) {
        return false;
    }

    return true;
}


bool DB::init() {
    char b[256];

    sprintf(b, SET_VERSION_FMT, USER_VERSION(format));
    if (sqlite3_exec(db, b, NULL, NULL, NULL) != SQLITE_OK)
        return false;

    if (sqlite3_exec(db, sql_db_init[format], NULL, NULL, NULL) != SQLITE_OK)
        return false;

    return true;
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
