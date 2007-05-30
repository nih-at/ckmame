/*
  $NiH$

  memdb.h -- in-memory sqlite3 db
  Copyright (C) 2007 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stddef.h>
#include <string.h>

#include "error.h"
#include "memdb.h"
#include "sq_util.h"



sqlite3 *memdb = NULL;
int memdb_inited = 0;

#define INSERT_PTR	\
    "insert into file_cache (name, pointer) values (?, ?)"
#define QUERY_PTR	\
    "select pointer from file_cache where name = ?"



const char *sql_db_init_mem = "\
create table file_cache (\n\
	game_id integer primary key,\n\
	name text not null,\n\
	pointer blob\n\
);\n\
create index file_cache_name on file_cache (name);\n\
\n\
create table file (\n\
	game_id integer,\n\
	file_type integer,\n\
	file_idx integer,\n\
	size integer,\n\
	crc integer,\n\
	md5 binray,\n\
	sha1 binary,\n\
	primary key (game_id, file_type, file_idx)\n\
);\n\
create index file_size on file (size);\n\
create index file_crc on file (crc);\n\
create index file_md5 on file (md5);\n\
create index file_sha1 on file (sha1);\n\
";



int
memdb_ensure(void)
{
    if (memdb_inited)
	return (memdb != NULL) ? 0 : -1;

    memdb_inited = 1;

    if (sqlite3_open(":memory:", &memdb) != SQLITE_OK
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

    if (memdb_ensure() < 0)
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
