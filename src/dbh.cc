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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include "dbh.h"
#include "xmalloc.h"

#ifndef EFTYPE
#define EFTYPE EINVAL
#endif

#define DBH_ENOERR 0
#define DBH_EVERSION 2 /* version mismatch */
#define DBH_EMAX 2

static const int format_version[] = {3, 1, 2};
#define USER_VERSION(fmt) (format_version[fmt] + (fmt << 8) + 17000)

#define SET_VERSION_FMT "pragma user_version = %d"

#define PRAGMAS "PRAGMA synchronous = OFF; "

static int init_db(dbh_t *);


static int
dbh_check_version(dbh_t *db) {
    sqlite3_stmt *stmt;
    int version;

    if ((stmt = dbh_get_statement(db, DBH_STMT_QUERY_VERSION)) == NULL) {
	/* TODO */
	return -1;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW) {
	/* TODO */
	return -1;
    }

    version = sqlite3_column_int(stmt, 0);

    if (version != USER_VERSION(db->format)) {
	db->dbh_errno = DBH_EVERSION;
	return -1;
    }

    db->dbh_errno = DBH_ENOERR;
    return 0;
}


int
dbh_close(dbh_t *db) {
    if (db == NULL)
	return 0;

    /* TODO finalize/free statements */

    if (dbh_db(db))
	return sqlite3_close(dbh_db(db));

    return 0;
}


const char *
dbh_error(dbh_t *db) {
    static const char *str[] = {"No error", "Database format version mismatch", "Unknown error"};

    if (db == NULL)
	return strerror(ENOMEM);

    /* TODO */
    if (db->dbh_errno == DBH_ENOERR)
	return sqlite3_errmsg(dbh_db(db));

    return str[db->dbh_errno < 0 || db->dbh_errno > DBH_EMAX ? DBH_EMAX : db->dbh_errno];
}


dbh_t *
dbh_open(const char *name, int mode) {
    dbh_t *db;
    struct stat st;
    unsigned int i;
    int sql3_flags;
    int needs_init = 0;

    if (DBH_FMT(mode) > sizeof(format_version) / sizeof(format_version[0])) {
	errno = EINVAL;
	return NULL;
    }

    if ((db = (dbh_t *)malloc(sizeof(*db))) == NULL)
	return NULL;

    db->db = NULL;
    for (i = 0; i < DBH_STMT_MAX; i++)
	db->statements[i] = NULL;

    db->format = DBH_FMT(mode);

    if (DBH_FLAGS(mode) & DBH_TRUNCATE) {
	/* do not delete special cases (like memdb) */
	if (name[0] != ':')
	    unlink(name);
	needs_init = 1;
    }

    if (DBH_FLAGS(mode) & DBH_WRITE)
	sql3_flags = SQLITE_OPEN_READWRITE;
    else
	sql3_flags = SQLITE_OPEN_READONLY;

    if (DBH_FLAGS(mode) & DBH_CREATE) {
	sql3_flags |= SQLITE_OPEN_CREATE;
	if (name[0] == ':' || (stat(name, &st) < 0 && errno == ENOENT))
	    needs_init = 1;
    }

    if (sqlite3_open_v2(name, &dbh_db(db), sql3_flags, NULL) != SQLITE_OK) {
	int save;
	save = errno;
	dbh_close(db);
	errno = save;
	return NULL;
    }

    if (sqlite3_exec(dbh_db(db), PRAGMAS, NULL, NULL, NULL) != SQLITE_OK) {
	int save;
	save = errno;
	dbh_close(db);
	errno = save;
	return NULL;
    }

    if (needs_init) {
	if (init_db(db) < 0) {
	    int save;
	    save = errno;
	    dbh_close(db);
	    unlink(name);
	    errno = save;
	    return NULL;
	}
    }
    else if (dbh_check_version(db) != 0) {
	dbh_close(db);
	errno = EFTYPE;
	return NULL;
    }

    return db;
}


static int
init_db(dbh_t *db) {
    char b[256];

    sprintf(b, SET_VERSION_FMT, USER_VERSION(db->format));
    if (sqlite3_exec(dbh_db(db), b, NULL, NULL, NULL) != SQLITE_OK)
	return -1;

    if (sqlite3_exec(dbh_db(db), sql_db_init[db->format], NULL, NULL, NULL) != SQLITE_OK)
	return -1;

    return 0;
}

dbh_stmt_t
dbh_stmt_with_hashes_and_size(dbh_stmt_t stmt, const hashes_t *hash, int have_size) {
    unsigned int i;

    for (i = 1; i <= HASHES_TYPE_MAX; i <<= 1) {
	if (hashes_has_type(hash, i))
	    stmt = static_cast<dbh_stmt_t>(stmt + i);
    }
    if (have_size)
	stmt = static_cast<dbh_stmt_t>(stmt + (HASHES_TYPE_MAX << 1));

    return stmt;
}

sqlite3_stmt *
dbh_get_statement(dbh_t *db, dbh_stmt_t stmt_id) {
    if (stmt_id >= DBH_STMT_MAX)
	return NULL;

    if (db->statements[stmt_id] == NULL) {
	if (sqlite3_prepare_v2(dbh_db(db), dbh_stmt_sql[stmt_id], -1, &(db->statements[stmt_id]), NULL) != SQLITE_OK) {
	    db->statements[stmt_id] = NULL;
	    return NULL;
	}
    }
    else {
	if (sqlite3_reset(db->statements[stmt_id]) != SQLITE_OK || sqlite3_clear_bindings(db->statements[stmt_id]) != SQLITE_OK) {
	    db->statements[stmt_id] = NULL;
            return NULL;
	}
    }

    return db->statements[stmt_id];
}
