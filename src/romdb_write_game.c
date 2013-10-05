/*
  romdb_write_game.c -- write game struct to db
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

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



/* write struct game to db */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "romdb.h"
#include "game.h"
#include "sq_util.h"
#include "util.h"

static int write_disks(romdb_t *, const game_t *);
static int write_rs(romdb_t *, const game_t *, filetype_t);



int
romdb_delete_game(romdb_t *db, const char *name)
{
    sqlite3_stmt *stmt;
    int ret, id;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_GAME_ID)) == NULL)
	return -1;

    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK)
	return -1;

    if ((ret=sqlite3_step(stmt)) == SQLITE_ROW)
	id = sqlite3_column_int(stmt, 0);

    if (ret == SQLITE_DONE)
	return 0;
    else if (ret != SQLITE_ROW)
	return -1;

    ret = 0;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_DELETE_GAME)) == NULL)
	return -1;
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_DONE)
	ret = -1;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_DELETE_FILE)) == NULL)
	return -1;
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_DONE)
	ret = -1;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_DELETE_PARENT)) == NULL)
	return -1;
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_DONE)
	ret = -1;

    return ret;
}



int
romdb_update_game(romdb_t *db, game_t *g)
{
    sqlite3_stmt *stmt;
    int ft, i;
    file_t *r;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_UPDATE_FILE)) == NULL)
	return -1;

    if (sqlite3_bind_int(stmt, 2, game_id(g)) != SQLITE_OK)
	return -1;

    for (ft=0; ft<GAME_RS_MAX; ft++) {
	if (sqlite3_bind_int(stmt, 3, ft) != SQLITE_OK)
	    return -1;

	for (i=0; i<game_num_files(g, ft); i++) {
	    r = game_file(g, ft, i);
	    if (file_where(r) == FILE_INZIP)
		continue;

	    if (sqlite3_bind_int(stmt, 1, file_where(r)) != SQLITE_OK
		|| sqlite3_bind_int(stmt, 4, i) != SQLITE_OK
		|| sqlite3_step(stmt) != SQLITE_DONE)
		return -1;
	}
    }

    return 0;
}



int
romdb_update_game_parent(romdb_t *db, game_t *g, filetype_t ft)
{
    sqlite3_stmt *stmt;
    dbh_stmt_t query;
    int off;

    if (game_cloneof(g, ft, 0)) {
	query = DBH_STMT_UPDATE_PARENT;
	off = 2;
    }
    else {
	query = DBH_STMT_DELETE_PARENT_FT;
	off = 1;
    }

    if ((stmt = dbh_get_statement(romdb_dbh(db), query)) == NULL)
	return -1;

    if (sqlite3_bind_int(stmt, off, game_id(g)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, off+1, ft) != SQLITE_OK)
	return -1;

    if (game_cloneof(g, ft, 0)
	&& sqlite3_bind_text(stmt, 1, game_cloneof(g, ft, 0), -1, SQLITE_STATIC) != SQLITE_OK)
	return -1;

    if (sqlite3_step(stmt) != SQLITE_DONE)
	return -1;

    return 0;
}




int
romdb_write_game(romdb_t *db, game_t *g)
{
    sqlite3_stmt *stmt;
    int i;

    romdb_delete_game(db, game_name(g));

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_INSERT_GAME)) == NULL)
	return -1;

    if (sq3_set_string(stmt, 1, game_name(g)) != SQLITE_OK
	|| sq3_set_string(stmt, 2, game_description(g)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 3, game_dat_no(g)) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_DONE)
	return -1;

    game_id(g) = sqlite3_last_insert_rowid(romdb_sqlite3(db));

    if (write_disks(db, g) < 0) {
	romdb_delete_game(db, game_name(g));
	return -1;
    }


    for (i=0; i<GAME_RS_MAX; i++) {
	if (write_rs(db, g, i) < 0) {
	    romdb_delete_game(db, game_name(g));
	    return -1;
	}
    }

    return 0;
}



static int
write_disks(romdb_t *db, const game_t *g)
{
    sqlite3_stmt *stmt;
    int i;
    disk_t *d;

    if (game_num_disks(g) == 0)
	return 0;
    
    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_INSERT_FILE)) == NULL)
	return -1;

    if (sqlite3_bind_int(stmt, 1, game_id(g)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, TYPE_DISK) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 7, 0) != SQLITE_OK)
	return -1;

    for (i=0; i<game_num_disks(g); i++) {
	d = game_disk(g, i);

	if (sqlite3_bind_int(stmt, 3, i) != SQLITE_OK
	    || sq3_set_string(stmt, 4, disk_name(d)) != SQLITE_OK
	    || sq3_set_string(stmt, 5, disk_merge(d)) != SQLITE_OK
	    || sqlite3_bind_int(stmt, 6, disk_status(d)) != SQLITE_OK
	    || sq3_set_hashes(stmt, 9, disk_hashes(d), 1) != SQLITE_OK
	    || sqlite3_step(stmt) != SQLITE_DONE
	    || sqlite3_reset(stmt) != SQLITE_OK)
	    return -1;
    }

    return 0;
}



static int
write_rs(romdb_t *db, const game_t *g, filetype_t ft)
{
    sqlite3_stmt *stmt;
    int i;
    file_t *r;

    if (game_cloneof(g, ft, 0)) {
	if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_INSERT_PARENT)) == NULL)
	    return -1;

	if (sqlite3_bind_int(stmt, 1, game_id(g)) != SQLITE_OK
	    || sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK
	    || sq3_set_string(stmt, 3, game_cloneof(g, ft, 0)) != SQLITE_OK
	    || sqlite3_step(stmt) != SQLITE_DONE)
	    return -1;
    }

    if (game_num_files(g, ft) == 0)
	return 0;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_INSERT_FILE)) == NULL)
	return -1;

    if (sqlite3_bind_int(stmt, 1, game_id(g)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK)
	return -1;

    for (i=0; i<game_num_files(g, ft); i++) {
	r = game_file(g, ft, i);

	if (sqlite3_bind_int(stmt, 3, i) != SQLITE_OK
	    || sq3_set_string(stmt, 4, file_name(r)) != SQLITE_OK
	    || sq3_set_string(stmt, 5, file_merge(r)) != SQLITE_OK
	    || sqlite3_bind_int(stmt, 6, file_status(r)) != SQLITE_OK
	    || sqlite3_bind_int(stmt, 7, file_where(r)) != SQLITE_OK
	    || sq3_set_int64_default(stmt, 8, file_size(r),
				     SIZE_UNKNOWN) != SQLITE_OK
	    || sq3_set_hashes(stmt, 9, file_hashes(r), 1) != SQLITE_OK
	    || sqlite3_step(stmt) != SQLITE_DONE
	    || sqlite3_reset(stmt) != SQLITE_OK)
	    return -1;
    }

    return 0;
}
