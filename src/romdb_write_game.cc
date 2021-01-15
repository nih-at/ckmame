/*
  romdb_write_game.c -- write game struct to db
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


/* write struct game to db */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "romdb.h"
#include "sq_util.h"
#include "util.h"

static int write_disks(romdb_t *, const Game *);
static int write_roms(romdb_t *, const Game *);


int
romdb_delete_game(romdb_t *db, const char *name) {
    sqlite3_stmt *stmt;
    int ret, id;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_GAME_ID)) == NULL)
	return -1;

    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK)
	return -1;

    if ((ret = sqlite3_step(stmt)) == SQLITE_ROW)
	id = sqlite3_column_int(stmt, 0);

    if (ret == SQLITE_DONE)
	return 0;
    else if (ret != SQLITE_ROW)
	return -1;

    ret = 0;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_DELETE_GAME)) == NULL)
	return -1;
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE)
	ret = -1;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_DELETE_FILE)) == NULL)
	return -1;
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE)
	ret = -1;

    return ret;
}


int
romdb_update_file_location(romdb_t *db, Game *game) {
    sqlite3_stmt *stmt;

    for (size_t i = 0; i < game->roms.size(); i++) {
	if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_UPDATE_FILE)) == NULL)
	    return -1;

	if (sqlite3_bind_int64(stmt, 2, game->id) != SQLITE_OK)
	    return -1;

	if (sqlite3_bind_int(stmt, 3, TYPE_ROM) != SQLITE_OK)
	    return -1;

	File *r = &game->roms[i];
	if (r->where == FILE_INGAME)
	    continue;

	if (sqlite3_bind_int(stmt, 1, r->where) != SQLITE_OK || sqlite3_bind_int(stmt, 4, i) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE)
	    return -1;
    }

    for (size_t i = 0; i < game->disks.size(); i++) {
	if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_UPDATE_FILE)) == NULL)
	    return -1;

	if (sqlite3_bind_int64(stmt, 2, game->id) != SQLITE_OK)
	    return -1;

	if (sqlite3_bind_int(stmt, 3, TYPE_DISK) != SQLITE_OK)
	    return -1;

        Disk *disk = &game->disks[i];
	if (disk->where == FILE_INGAME)
	    continue;

	if (sqlite3_bind_int(stmt, 1, disk->where) != SQLITE_OK || sqlite3_bind_int(stmt, 4, i) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE)
	    return -1;
    }

    return 0;
}


int
romdb_update_game_parent(romdb_t *db, const Game *game) {
    sqlite3_stmt *stmt;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_UPDATE_PARENT)) == NULL || sq3_set_string(stmt, 1, game->cloneof[0].c_str()) != SQLITE_OK || sqlite3_bind_int64(stmt, 2, game->id) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE) {
	return -1;
    }

    return 0;
}


int
romdb_write_game(romdb_t *db, Game *game) {
    sqlite3_stmt *stmt;

    romdb_delete_game(db, game->name.c_str());

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_INSERT_GAME)) == NULL)
	return -1;

    if (sq3_set_string(stmt, 1, game->name.c_str()) != SQLITE_OK || sq3_set_string(stmt, 2, game->description.c_str()) != SQLITE_OK || sqlite3_bind_int(stmt, 3, game->dat_no) != SQLITE_OK || sq3_set_string(stmt, 4, game->cloneof[0].c_str()) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE)
	return -1;

    game->id = sqlite3_last_insert_rowid(romdb_sqlite3(db));

    if (write_disks(db, game) < 0) {
	romdb_delete_game(db, game->name.c_str());
	return -1;
    }

    if (write_roms(db, game) < 0) {
	romdb_delete_game(db, game->name.c_str());
	return -1;
    }

    return 0;
}


static int
write_disks(romdb_t *db, const Game *game) {
    sqlite3_stmt *stmt;

    for (size_t i = 0; i < game->disks.size(); i++) {
        auto &disk = game->disks[i];

	if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_INSERT_FILE)) == NULL)
	    return -1;

	if (sqlite3_bind_int64(stmt, 1, game->id) != SQLITE_OK || sqlite3_bind_int(stmt, 2, TYPE_DISK) != SQLITE_OK)
	    return -1;

	if (sqlite3_bind_int(stmt, 3, i) != SQLITE_OK || sq3_set_string(stmt, 4, disk.name.c_str()) != SQLITE_OK || sq3_set_string(stmt, 5, disk.merge.c_str()) != SQLITE_OK || sqlite3_bind_int(stmt, 6, disk.status) != SQLITE_OK || sqlite3_bind_int(stmt, 7, disk.where) != SQLITE_OK || sq3_set_hashes(stmt, 9, &disk.hashes, 1) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE || sqlite3_reset(stmt) != SQLITE_OK)
	    return -1;
    }

    return 0;
}


static int
write_roms(romdb_t *db, const Game *game) {
    sqlite3_stmt *stmt;

    for (size_t i = 0; i < game->roms.size(); i++) {
        const File *r = &game->roms[i];

	if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_INSERT_FILE)) == NULL)
	    return -1;

	if (sqlite3_bind_int64(stmt, 1, game->id) != SQLITE_OK || sqlite3_bind_int(stmt, 2, TYPE_ROM) != SQLITE_OK)
	    return -1;

	if (sqlite3_bind_int(stmt, 3, i) != SQLITE_OK || sq3_set_string(stmt, 4, r->name.c_str()) != SQLITE_OK || sq3_set_string(stmt, 5, r->merge.c_str()) != SQLITE_OK || sqlite3_bind_int(stmt, 6, r->status) != SQLITE_OK || sqlite3_bind_int(stmt, 7, r->where) != SQLITE_OK || sq3_set_int64_default(stmt, 8, r->size, SIZE_UNKNOWN) != SQLITE_OK || sq3_set_hashes(stmt, 9, &r->hashes, 1) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE || sqlite3_reset(stmt) != SQLITE_OK)
	    return -1;
    }

    return 0;
}
