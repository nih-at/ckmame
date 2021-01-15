/*
  memdb.c -- in-memory sqlite3 db
  Copyright (C) 2007-2014 Dieter Baron and Thomas Klausner

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "memdb.h"
#include "sq_util.h"


dbh_t *memdb = NULL;
int memdb_inited = 0;

#define INSERT_FILE_GAME_ID 1
#define INSERT_FILE_FILE_TYPE 2
#define INSERT_FILE_FILE_IDX 3
#define INSERT_FILE_FILE_SH 4
#define INSERT_FILE_LOCATION 5
#define INSERT_file_size_ 6
#define INSERT_FILE_HASHES 7


static int _delete_file(uint64_t, filetype_t, int);
static int _update_file(uint64_t, filetype_t, int, const Hashes *);


int
memdb_ensure(void) {
    const char *dbname;

    if (memdb_inited)
	return (memdb != NULL) ? 0 : -1;

    if (getenv("CKMAME_DEBUG_MEMDB")) {
	dbname = "memdb.sqlite3";
    }
    else
	dbname = ":memory:";

    memdb_inited = 1;

    if ((memdb = dbh_open(dbname, DBH_FMT_MEM | DBH_NEW)) == NULL) {
	myerror(ERRSTR, "cannot create in-memory db");
	return -1;
    }

    return 0;
}


void *
memdb_get_ptr(const char *name, filetype_t type) {
    sqlite3_stmt *stmt;
    void *ptr;

    if (!memdb_inited)
	return NULL;

    if ((stmt = dbh_get_statement(memdb, DBH_STMT_MEM_QUERY_PTR)) == NULL) {
	seterrdb(memdb);
	myerror(ERRDB, "cannot get '%s' from file cache", name);
	return NULL;
    }

    if (sq3_set_string(stmt, 1, name) != SQLITE_OK || sqlite3_bind_int(stmt, 2, type) != SQLITE_OK) {
	seterrdb(memdb);
	myerror(ERRDB, "cannot get '%s' from file cache", name);
	return NULL;
    }

    switch (sqlite3_step(stmt)) {
    case SQLITE_ROW:
	memcpy(&ptr, sqlite3_column_blob(stmt, 0), sizeof(ptr));
	break;

    case SQLITE_DONE:
	ptr = NULL;
	break;

    default:
	ptr = NULL;
	seterrdb(memdb);
	myerror(ERRDB, "cannot get '%s' from file cache", name);
    }

    return ptr;
}


void *
memdb_get_ptr_by_id(int id) {
    sqlite3_stmt *stmt;
    void *ptr;

    if (!memdb_inited)
	return NULL;

    if ((stmt = dbh_get_statement(memdb, DBH_STMT_MEM_QUERY_PTR_ID)) == NULL) {
	seterrdb(memdb);
	myerror(ERRDB, "cannot get '%d' from file cache", id);
	return NULL;
    }

    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK) {
	seterrdb(memdb);
	myerror(ERRDB, "cannot get '%d' from file cache", id);
	return NULL;
    }

    switch (sqlite3_step(stmt)) {
    case SQLITE_ROW:
	memcpy(&ptr, sqlite3_column_blob(stmt, 0), sizeof(ptr));
	break;

    case SQLITE_DONE:
	ptr = NULL;
	break;

    default:
	ptr = NULL;
	seterrdb(memdb);
	myerror(ERRDB, "cannot get '%d' from file cache", id);
    }

    return ptr;
}


int64_t
memdb_put_ptr(const char *name, filetype_t type, void *ptr) {
    sqlite3_stmt *stmt;
    int64_t ret;

    if (memdb_ensure() < 0)
	return -1;

    if ((stmt = dbh_get_statement(memdb, DBH_STMT_MEM_INSERT_PTR)) == NULL)
	ret = -1;
    else {
	if (sq3_set_string(stmt, 1, name) != SQLITE_OK || sqlite3_bind_int(stmt, 2, type) != SQLITE_OK || sqlite3_bind_blob(stmt, 3, &ptr, sizeof(void *), SQLITE_STATIC) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE) {
	    ret = -1;
	}
	else
	    ret = sqlite3_last_insert_rowid(dbh_db(memdb));
    }

    if (ret < 0) {
	seterrdb(memdb);
	myerror(ERRDB, "cannot insert '%s' into file cache", name);
    }

    return ret;
}


int
memdb_file_delete(const Archive *a, int idx, bool adjust_idx) {
    sqlite3_stmt *stmt;

    if (_delete_file(a->id, a->filetype, idx) < 0)
	return -1;

    if (!adjust_idx)
	return 0;

    if ((stmt = dbh_get_statement(memdb, DBH_STMT_MEM_DEC_FILE_IDX)) == NULL)
	return -1;

    if (sqlite3_bind_int64(stmt, 1, a->id) != SQLITE_OK || sqlite3_bind_int(stmt, 2, a->filetype) != SQLITE_OK || sqlite3_bind_int(stmt, 3, idx) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE)
	return -1;

    return 0;
}


int
memdb_file_insert(sqlite3_stmt *stmt, const Archive *a, int idx) {
    int i, err;

    if (memdb_ensure() < 0)
	return -1;

    auto r = &a->files[idx];

    if (stmt == NULL) {
	if ((stmt = dbh_get_statement(memdb, DBH_STMT_MEM_INSERT_FILE)) == NULL)
	    return -1;

	if (sqlite3_bind_int64(stmt, INSERT_FILE_GAME_ID, a->id) != SQLITE_OK || sqlite3_bind_int(stmt, INSERT_FILE_FILE_TYPE, a->filetype) != SQLITE_OK || sqlite3_bind_int(stmt, INSERT_FILE_LOCATION, a->where) != SQLITE_OK)
	    return -1;
    }

    err = 0;

    if (sqlite3_bind_int(stmt, INSERT_FILE_FILE_IDX, idx) != SQLITE_OK)
	err = -1;
    else {
	for (i = 0; i < 2; i++) {
	    bool detector = (i == 1);
	    if (detector && !r->size_hashes_are_set(detector)) {
		continue;
	    }

	    if (sqlite3_bind_int(stmt, INSERT_FILE_FILE_SH, i) != SQLITE_OK || sq3_set_int64_default(stmt, INSERT_file_size_, r->get_size(detector), SIZE_UNKNOWN) != SQLITE_OK || sq3_set_hashes(stmt, INSERT_FILE_HASHES, &r->get_hashes(detector), 1) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE || sqlite3_reset(stmt) != SQLITE_OK) {
		err = -1;
		continue;
	    }
	}
    }

    return err;
}


int
memdb_file_insert_archive(const Archive *archive) {
    sqlite3_stmt *stmt;
    int err;

    if (memdb_ensure() < 0)
	return -1;

    if ((stmt = dbh_get_statement(memdb, DBH_STMT_MEM_INSERT_FILE)) == NULL)
	return -1;

    if (sqlite3_bind_int64(stmt, INSERT_FILE_GAME_ID, archive->id) != SQLITE_OK || sqlite3_bind_int(stmt, INSERT_FILE_FILE_TYPE, archive->filetype) != SQLITE_OK || sqlite3_bind_int(stmt, INSERT_FILE_LOCATION, archive->where) != SQLITE_OK)
	return -1;

    err = 0;
    for (size_t i = 0; i < archive->files.size(); i++) {
        if (archive->files[i].status != STATUS_OK)
	    continue;
	if (memdb_file_insert(stmt, archive, i) < 0)
	    err = -1;
    }

    return err;
}


int
memdb_update_disk(const Disk *disk) {
    return _update_file(disk->id, TYPE_DISK, 0, &disk->hashes);
}


int
memdb_update_file(const Archive *archive, int idx) {
    if (archive->files[idx].status != STATUS_OK)
	return _delete_file(archive->id, archive->filetype, idx);

    return _update_file(archive->id, archive->filetype, idx, &archive->files[idx].hashes);
}


static int
_update_file(uint64_t id, filetype_t ft, int idx, const Hashes *h) {
    sqlite3_stmt *stmt;

    /* FILE_SH_DETECTOR hashes are always completely filled in */

    if ((stmt = dbh_get_statement(memdb, DBH_STMT_MEM_UPDATE_FILE)) == NULL)
	return -1;

    if (sq3_set_hashes(stmt, 1, h, 1) != SQLITE_OK || sqlite3_bind_int64(stmt, 4, id) != SQLITE_OK || sqlite3_bind_int(stmt, 5, ft) != SQLITE_OK || sqlite3_bind_int(stmt, 6, idx) != SQLITE_OK || sqlite3_bind_int(stmt, 7, 0) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE)
	return -1;

    return 0;
}


static int
_delete_file(uint64_t id, filetype_t ft, int idx) {
    sqlite3_stmt *stmt;

    if ((stmt = dbh_get_statement(memdb, DBH_STMT_MEM_DELETE_FILE)) == NULL)
	return -1;

    if (sqlite3_bind_int64(stmt, 1, id) != SQLITE_OK || sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK || sqlite3_bind_int(stmt, 3, idx) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE)
	return -1;

    return 0;
}
