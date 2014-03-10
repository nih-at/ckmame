/*
 dbh_dir.c -- files in dirs sqlite3 data base
 Copyright (C) 2014 Dieter Baron and Thomas Klausner
 
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

#include <stddef.h>

#include "dbh_dir.h"
#include "error.h"
#include "funcs.h"
#include "sq_util.h"
#include "util.h"
#include "xmalloc.h"

static dbh_t *romset_db;
static int romset_db_initialized;

static int dbh_dir_write_file_with_stmt(int id, const file_t *f, sqlite3_stmt *stmt);


int
dbh_dir_close(void)
{
    if (romset_db)
	return dbh_close(romset_db);

    return 0;
}


int
dbh_dir_delete(int id)
{
    sqlite3_stmt *stmt;

    if (dbh_dir_delete_files(id) < 0)
        return -1;
    
    if ((stmt = dbh_get_statement(romset_db, DBH_STMT_DIR_DELETE_ARCHIVE)) == NULL)
	return -1;
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE)
	return -1;

    return 0;
}


int dbh_dir_delete_files(int id)
{
    sqlite3_stmt *stmt;

    if ((stmt = dbh_get_statement(romset_db, DBH_STMT_DIR_DELETE_FILE)) == NULL)
	return -1;
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE)
	return -1;

    return 0;
}


int
dbh_dir_read(const char *name, array_t *files)
{
    sqlite3_stmt *stmt;
    int ret;
    int archive_id;

    if ((stmt = dbh_get_statement(romset_db, DBH_STMT_DIR_QUERY_ARCHIVE)) == NULL)
	return -1;

    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK)
	return -1;
    if (sqlite3_step(stmt) != SQLITE_ROW)
	return 0;
    archive_id = sqlite3_column_int(stmt, 0);

    if ((stmt = dbh_get_statement(romset_db, DBH_STMT_DIR_QUERY_FILE)) == NULL)
	return -1;
    if (sqlite3_bind_int(stmt, 1, archive_id) != SQLITE_OK)
	return -1;

    array_truncate(files, 0, file_finalize);

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	file_t *f = (file_t *)array_grow(files, file_init);

	file_name(f) = sq3_get_string(stmt, 0);
	file_mtime(f) = sqlite3_column_int(stmt, 1);
	file_status(f) = sqlite3_column_int(stmt, 2);
	file_size(f) = sq3_get_int64_default(stmt, 3, SIZE_UNKNOWN);
	sq3_get_hashes(file_hashes(f), stmt, 4);
    }

    if (ret != SQLITE_DONE) {
	array_truncate(files, 0, file_finalize);
	return -1;
    }

    return archive_id;
}


int
dbh_dir_write(int id, const char *name, array_t *files)
{
    sqlite3_stmt *stmt;
    int i;

    if (id != 0) {
	if (dbh_dir_delete(id) < 0)
	    return -1;
    }
    
    if ((id = dbh_dir_write_archive(id, name)) < 0)
        return -1;
    

    if ((stmt = dbh_get_statement(romset_db, DBH_STMT_DIR_INSERT_FILE)) == NULL)
	return -1;

    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK)
        return -1;

    for (i=0; i<array_length(files); i++) {
        if (dbh_dir_write_file_with_stmt(id, array_get(files, i), stmt) < 0)
	    return -1;
    }

    return id;
}

int dbh_dir_write_archive(int id, const char *name)
{
    sqlite3_stmt *stmt;

    stmt = dbh_get_statement(romset_db, DBH_STMT_DIR_INSERT_ARCHIVE_ID);
    
    if (stmt == NULL || sq3_set_string(stmt, 1, name) != SQLITE_OK)
	return -1;
    
    if (id != 0) {
	if (sqlite3_bind_int(stmt, 2, id) != SQLITE_OK)
	    return -1;
    }
    
    if (sqlite3_step(stmt) != SQLITE_DONE)
	return -1;
    
    if (id == 0)
	id = (int)sqlite3_last_insert_rowid(dbh_db(romset_db)); /* TODO: use int64_t as id */
    
    return id;
}

int dbh_dir_write_file(int id, const file_t *f)
{
    sqlite3_stmt *stmt;

    if ((stmt = dbh_get_statement(romset_db, DBH_STMT_DIR_INSERT_FILE)) == NULL)
	return -1;
    
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK)
        return -1;

    return dbh_dir_write_file_with_stmt(id, f, stmt);
}


static int dbh_dir_write_file_with_stmt(int id, const file_t *f, sqlite3_stmt *stmt)
{
    if (sq3_set_string(stmt, 2, file_name(f)) != SQLITE_OK
        || sqlite3_bind_int(stmt, 3, file_mtime(f)) != SQLITE_OK
        || sqlite3_bind_int(stmt, 4, file_status(f)) != SQLITE_OK
        || sq3_set_int64_default(stmt, 5, file_size(f), SIZE_UNKNOWN) != SQLITE_OK
        || sq3_set_hashes(stmt, 6, file_hashes(f), 1) != SQLITE_OK
        || sqlite3_step(stmt) != SQLITE_DONE
        || sqlite3_reset(stmt) != SQLITE_OK)
        return -1;
 
    return 0;
}

int
ensure_romset_dir_db(void)
{
    char *dbname = NULL;
    
    if (romset_db_initialized)
	return romset_db ? 0 : -1;

    romset_db_initialized = 1;

    if (ensure_dir(get_directory(TYPE_ROM), 0) < 0)
	return -1;

    if (xasprintf(&dbname, "%s/%s", get_directory(TYPE_ROM), DBH_DIR_DB_NAME) < 0) {
	myerror(ERRSTR, "vasprintf failed");
	return -1;
    }

    if ((romset_db=dbh_open(dbname, DBH_FMT_DIR|DBH_CREATE|DBH_WRITE)) == NULL) {
	myerror(ERRDB, "can't open rom directory database for '%s'", get_directory(TYPE_ROM));
	free(dbname);
	return -1;
    }
    
    free(dbname);

    return 0;
}
