/*
  romdb_read_game.c -- read game struct from db
  Copyright (C) 1999-2013 Dieter Baron and Thomas Klausner

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



/* read struct game from db */

#include <string.h>
#include <stdlib.h>

#include "game.h"
#include "romdb.h"
#include "sq_util.h"
#include "xmalloc.h"

static int read_disks(romdb_t *, game_t *);
static int read_rs(romdb_t *, game_t *, filetype_t);



game_t *
romdb_read_game(romdb_t *db, const char *name)
{
    sqlite3_stmt *stmt;
    game_t *game;
    int i;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_GAME)) == NULL)
	return NULL;

    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_ROW)
	return NULL;

    game = game_new();
    game->id = sqlite3_column_int(stmt, 0);
    game->name = xstrdup(name);
    game->description = sq3_get_string(stmt, 1);
    game->dat_no = sqlite3_column_int(stmt, 2);

    for (i=0; i<GAME_RS_MAX; i++) {
	if (read_rs(db, game, i) < 0) {
	    game_free(game);
	    return NULL;
	}
    }

    if (read_disks(db, game) < 0) {
	game_free(game);
	return NULL;
    }

    return game;
}



static int
read_disks(romdb_t *db, game_t *g)
{
    sqlite3_stmt *stmt;
    int ret;
    disk_t *d;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_FILE)) == NULL)
	return -1;

    if (sqlite3_bind_int(stmt, 1, game_id(g)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, TYPE_DISK) != SQLITE_OK)
	return -1;

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	d = (disk_t *)array_grow(game_disks(g), disk_init);

	disk_name(d) = sq3_get_string(stmt, 0);
	disk_merge(d) = sq3_get_string(stmt, 1);
	disk_status(d) = sqlite3_column_int(stmt, 2);
	sq3_get_hashes(disk_hashes(d), stmt, 5);
    }

    return (ret == SQLITE_DONE ? 0 : -1);
}



static int
read_rs(romdb_t *db, game_t *g, filetype_t ft)
{
    sqlite3_stmt *stmt;
    int ret;
    file_t *r;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_PARENT)) == NULL)
	return -1;
    if (sqlite3_bind_int(stmt, 1, game_id(g)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK)
	return -1;

    if ((ret=sqlite3_step(stmt)) == SQLITE_ROW)
	game_cloneof(g, ft, 0) = sq3_get_string(stmt, 0);

    if (ret != SQLITE_ROW && ret != SQLITE_DONE)
	return -1;

    if (game_cloneof(g, ft, 0)) {
	if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_GPARENT)) == NULL)
	    return -1;
	if (sq3_set_string(stmt, 1, game_cloneof(g, ft, 0)) != SQLITE_OK
	    || sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK)
	    return -1;

	if ((ret=sqlite3_step(stmt)) == SQLITE_ROW)
	    game_cloneof(g, ft, 1) = sq3_get_string(stmt, 0);
	
	if (ret != SQLITE_ROW && ret != SQLITE_DONE)
	    return -1;
    }

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_FILE)) == NULL)
	return -1;

    if (sqlite3_bind_int(stmt, 1, game_id(g)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK)
	return -1;

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	r = (file_t *)array_grow(game_files(g, ft), file_init);

	file_name(r) = sq3_get_string(stmt, 0);
	file_merge(r) = sq3_get_string(stmt, 1);
	file_status(r) = sqlite3_column_int(stmt, 2);
	file_where(r) = sqlite3_column_int(stmt, 3);
	file_size(r) = sq3_get_int64_default(stmt, 4, SIZE_UNKNOWN);
	sq3_get_hashes(file_hashes(r), stmt, 5);
    }

    return (ret == SQLITE_DONE ? 0 : -1);
}
