/*
  romdb.c -- mame.db sqlite3 data base
  Copyright (C) 2013-2014 Dieter Baron and Thomas Klausner

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
#include <stdlib.h>

#include "romdb.h"
#include "xmalloc.h"

const dbh_stmt_t query_hash_type[] = {DBH_STMT_QUERY_HASH_TYPE_CRC, DBH_STMT_QUERY_HASH_TYPE_MD5, DBH_STMT_QUERY_HASH_TYPE_SHA1};

static void read_hashtypes_ft(romdb_t *, filetype_t);


int
romdb_close(romdb_t *db) {
    int ret = dbh_close(romdb_dbh(db));

    free(db);

    return ret;
}


int
romdb_has_disks(romdb_t *db) {
    sqlite3_stmt *stmt = dbh_get_statement(db->dbh, DBH_STMT_QUERY_HAS_DISKS);
    if (stmt == NULL) {
	return -1;
    }

    switch (sqlite3_step(stmt)) {
    case SQLITE_ROW:
	return 1;

    case SQLITE_DONE:
	return 0;

    default:
	return -1;
    }
}


int
romdb_hashtypes(romdb_t *db, filetype_t type) {
    if (type >= TYPE_MAX) {
	errno = EINVAL;
	return -1;
    }

    if (db->hashtypes[type] == -1)
	read_hashtypes_ft(db, type);

    return db->hashtypes[type];
}


romdb_t *
romdb_open(const char *name, int mode) {
    dbh_t *dbh = dbh_open(name, mode);

    if (dbh == NULL)
	return NULL;

    romdb_t *db = xmalloc(sizeof(*db));

    db->dbh = dbh;

    int i;
    for (i = 0; i < TYPE_MAX; i++) {
	db->hashtypes[i] = -1;
    }

    return db;
}


static void
read_hashtypes_ft(romdb_t *db, filetype_t ft) {
    int type;
    sqlite3_stmt *stmt;

    db->hashtypes[ft] = 0;

    for (type = 0; (1 << type) <= HASHES_TYPE_MAX; type++) {
	if ((stmt = dbh_get_statement(romdb_dbh(db), query_hash_type[type])) == NULL)
	    continue;
	if (sqlite3_bind_int(stmt, 1, ft) != SQLITE_OK)
	    continue;
	if (sqlite3_step(stmt) == SQLITE_ROW)
	    db->hashtypes[ft] |= (1 << type);
    }
}
