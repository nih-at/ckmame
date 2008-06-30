/*
  memdb.h -- in-memory sqlite3 db
  Copyright (C) 2007-2008 Dieter Baron and Thomas Klausner

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



sqlite3 *memdb = NULL;
int memdb_inited = 0;

#define INSERT_PTR	\
    "insert into ptr_cache (name, pointer) values (?, ?)"
#define QUERY_PTR	\
    "select pointer from ptr_cache where name = ?"
#define QUERY_PTR_ID	\
    "select pointer from ptr_cache where game_id = ?"

#define INSERT_FILE							\
    "insert into file (game_id, file_type, file_idx, file_sh, location," \
    " size, crc, md5, sha1) values (?, ?, ?, ?, ?, ?, ?, ?, ?)"
#define INSERT_FILE_GAME_ID	1
#define INSERT_FILE_FILE_TYPE	2
#define INSERT_FILE_FILE_IDX	3
#define INSERT_FILE_FILE_SH	4
#define INSERT_FILE_LOCATION	5
#define INSERT_FILE_SIZE	6
#define INSERT_FILE_HASHES	7

#define UPDATE_FILE \
    "update file set crc = ?, md5 = ?, sha1 = ? where" \
    " game_id = ? and file_type = ? and file_idx = ? and file_sh = ?"
#define DELETE_FILE \
    "delete from file where" \
    " game_id = ? and file_type = ? and file_idx = ?"
#define DEC_FILE_IDX \
    "update file set idx=idx-1 where" \
    " game_id = ? and file_type = ? and file_idx > ?"



const char *sql_db_init_mem = "\
create table ptr_cache (\n\
	game_id integer primary key,\n\
	name text not null,\n\
	pointer blob\n\
);\n\
create index ptr_cache_name on ptr_cache (name);\n\
\n\
create table file (\n\
	game_id integer,\n\
	file_type integer,\n\
	file_idx integer,\n\
	file_sh integer,\n\
	location integer not null,\n\
	size integer,\n\
	crc integer,\n\
	md5 binray,\n\
	sha1 binary\n\
);\n\
create index file_id on file (game_id, file_type, file_idx);\n\
create index file_location on file (location);\n\
create index file_size on file (size);\n\
create index file_crc on file (crc);\n\
create index file_md5 on file (md5);\n\
create index file_sha1 on file (sha1);\n\
";



static int _delete_file(int, filetype_t, int);
static int _update_file(int, filetype_t, int, const hashes_t *);



int
memdb_ensure(void)
{
    char *dbname;

    if (memdb_inited)
	return (memdb != NULL) ? 0 : -1;

    if (getenv("CKMAME_DEBUG_MEMDB")) {
	dbname = "memdb.sqlite3";
	remove(dbname);
    }
    else
	dbname = ":memory:";

    memdb_inited = 1;

    if (sqlite3_open(dbname, &memdb) != SQLITE_OK
	|| sqlite3_exec(memdb, sql_db_init_mem, NULL, NULL,
			NULL) != SQLITE_OK) {
	seterrdb(memdb);
	myerror(ERRDB, "cannot create in-memory db");
	sqlite3_close(memdb);
	memdb = NULL;
	return -1;
    }

    return 0;
}



void *
memdb_get_ptr(const char *name)
{
    sqlite3_stmt *stmt;
    void *ptr;

    if (!memdb_inited)
	return NULL;

    if (sqlite3_prepare_v2(memdb, QUERY_PTR, -1, &stmt, NULL) != SQLITE_OK) {
	seterrdb(memdb);
	myerror(ERRDB, "cannot get `%s' from file cache", name);
	return NULL;
    }

    if (sq3_set_string(stmt, 1, name) != SQLITE_OK) {
	seterrdb(memdb);
	myerror(ERRDB, "cannot get `%s' from file cache", name);
	sqlite3_finalize(stmt);
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
	myerror(ERRDB, "cannot get `%s' from file cache", name);
    }
    sqlite3_finalize(stmt);

    return ptr;
}



void *
memdb_get_ptr_by_id(int id)
{
    sqlite3_stmt *stmt;
    void *ptr;

    if (!memdb_inited)
	return NULL;

    if (sqlite3_prepare_v2(memdb, QUERY_PTR_ID, -1, &stmt, NULL) != SQLITE_OK) {
	seterrdb(memdb);
	myerror(ERRDB, "cannot get `%d' from file cache", id);
	return NULL;
    }

    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK) {
	seterrdb(memdb);
	myerror(ERRDB, "cannot get `%d' from file cache", id);
	sqlite3_finalize(stmt);
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
	myerror(ERRDB, "cannot get `%d' from file cache", id);
    }
    sqlite3_finalize(stmt);

    return ptr;
}



int
memdb_put_ptr(const char *name, void *ptr)
{
    sqlite3_stmt *stmt;
    int ret;

    if (memdb_ensure() < 0)
	return -1;

    if (sqlite3_prepare_v2(memdb, INSERT_PTR, -1, &stmt, NULL) == SQLITE_OK) {
	if (sq3_set_string(stmt, 1, name) != SQLITE_OK
	    || sqlite3_bind_blob(stmt, 2, &ptr, sizeof(void *),
				 SQLITE_STATIC) != SQLITE_OK
	    || sqlite3_step(stmt) != SQLITE_DONE) {
	    ret = -1;
	}
	else
	    ret = sqlite3_last_insert_rowid(memdb);
	sqlite3_finalize(stmt);
    }
    else
	ret = -1;

    if (ret < 0) {
	seterrdb(memdb);
	myerror(ERRDB, "cannot insert `%s' into file cache", name);
    }

    return ret;
}



int
memdb_file_delete(const archive_t *a, int idx, bool adjust_idx)
{
    sqlite3_stmt *stmt;

    if (_delete_file(archive_id(a), archive_filetype(a), idx) < 0)
	return -1;

    if (!adjust_idx)
	return 0;
    
    if (sqlite3_prepare_v2(memdb, DEC_FILE_IDX, -1, &stmt, NULL) != SQLITE_OK)
	return -1;
    
    if (sqlite3_bind_int(stmt, 1, archive_id(a)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, archive_filetype(a)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 3, idx) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_DONE) {
	sqlite3_finalize(stmt);
	return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}



int
memdb_file_insert(sqlite3_stmt *stmt, const archive_t *a, int idx)
{
    bool stmt_owned;
    file_t *r;
    int i, err;

    if (memdb_ensure() < 0)
	return -1;

    r = archive_file(a, idx);

    if (stmt)
	stmt_owned = false;
    else {
	if (sqlite3_prepare_v2(memdb, INSERT_FILE, -1,
			       &stmt, NULL) != SQLITE_OK)
	    return -1;

	if (sqlite3_bind_int(stmt, INSERT_FILE_GAME_ID,
			     archive_id(a)) != SQLITE_OK
	    || sqlite3_bind_int(stmt, INSERT_FILE_FILE_TYPE,
				archive_filetype(a)) != SQLITE_OK
	    || sqlite3_bind_int(stmt, INSERT_FILE_LOCATION,
				archive_where(a)) != SQLITE_OK) {
	    sqlite3_finalize(stmt);
	    return -1;
	}

	stmt_owned = true;
    }

    err = 0;

    if (sqlite3_bind_int(stmt, INSERT_FILE_FILE_IDX, idx) != SQLITE_OK)
	err = -1;
    else {
	for (i=0; i<FILE_SH_MAX; i++) {
	    if (!file_sh_is_set(r, i) && i != FILE_SH_FULL)
		continue;

	    if (sqlite3_bind_int(stmt, INSERT_FILE_FILE_SH, i) != SQLITE_OK
		|| sq3_set_int64_default(stmt, INSERT_FILE_SIZE,
					 file_size_xxx(r, i),
					 SIZE_UNKNOWN) != SQLITE_OK
		|| sq3_set_hashes(stmt, INSERT_FILE_HASHES,
				  file_hashes_xxx(r, i), 1) != SQLITE_OK
		|| sqlite3_step(stmt) != SQLITE_DONE
		|| sqlite3_reset(stmt) != SQLITE_OK) {
		err = -1;
		continue;
	    }
	}
    }

    if (stmt_owned) {
	if (sqlite3_finalize(stmt) != SQLITE_OK)
	    err = -1;
    }

    return err;
}



int
memdb_file_insert_archive(const archive_t *a)
{
    sqlite3_stmt *stmt;
    int i, err;

    if (memdb_ensure() < 0)
	return -1;

    if (sqlite3_prepare_v2(memdb, INSERT_FILE, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    if (sqlite3_bind_int(stmt, INSERT_FILE_GAME_ID, archive_id(a)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, INSERT_FILE_FILE_TYPE,
			    archive_filetype(a)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, INSERT_FILE_LOCATION,
			    archive_where(a)) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return -1;
    }

    err = 0;
    for (i=0; i<archive_num_files(a); i++) {
	if (file_status(archive_file(a, i)) != STATUS_OK)
	    continue;
	if (memdb_file_insert(stmt, a, i) < 0)
	    err = -1;
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK)
	err = -1;

    return err;
}



int
memdb_update_disk(const disk_t *d)
{
    return _update_file(disk_id(d), TYPE_DISK, 0, disk_hashes(d));
}



int
memdb_update_file(const archive_t *a, int idx)
{
    if (file_status(archive_file(a, idx)) != STATUS_OK)
	return _delete_file(archive_id(a), archive_filetype(a), idx);
    
    return _update_file(archive_id(a), archive_filetype(a), idx,
			file_hashes(archive_file(a, idx)));
}



static int
_update_file(int id, filetype_t ft, int idx, const hashes_t *h)
{
    sqlite3_stmt *stmt;

    /* FILE_SH_DETECTOR hashes are always completely filled in */

    if (sqlite3_prepare_v2(memdb, UPDATE_FILE, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    if (sq3_set_hashes(stmt, 1, h, 1) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 4, id) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 5, ft) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 6, idx) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 7, FILE_SH_FULL) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_DONE) {
	sqlite3_finalize(stmt);
	return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}



static int
_delete_file(int id, filetype_t ft, int idx)
{
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(memdb, DELETE_FILE, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 3, idx) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_DONE) {
	sqlite3_finalize(stmt);
	return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}
