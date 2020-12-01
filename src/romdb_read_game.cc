/*
  romdb_read_game.c -- read game struct from db
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


/* read struct game from db */

#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "romdb.h"
#include "sq_util.h"
#include "xmalloc.h"

static int read_disks(romdb_t *, Game *);
static int read_roms(romdb_t *, Game *);


GamePtr
romdb_read_game(romdb_t *db, const char *name) {
    sqlite3_stmt *stmt;
    int ret;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_GAME)) == NULL) {
	return NULL;
    }

    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_ROW) {
	return NULL;
    }

    auto game = std::make_shared<Game>();
    game->id = sqlite3_column_int(stmt, 0);
    game->name = xstrdup(name);
    game->description = sq3_get_string(stmt, 1);
    game->dat_no = sqlite3_column_int(stmt, 2);
    game->cloneof[0] = sq3_get_string(stmt, 3);

    if (!game->cloneof[0].empty()) {
        if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_PARENT_BY_NAME)) == NULL || sq3_set_string(stmt, 1, game->cloneof[0].c_str()) != SQLITE_OK) {
            return NULL;
        }
        
        if ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
            game->cloneof[1] = sq3_get_string(stmt, 0);
        }
        
        if (ret != SQLITE_ROW && ret != SQLITE_DONE) {
            return NULL;
        }
    }

    if (read_roms(db, game.get()) < 0) {
        // TODO: use error reporting function
        printf("can't read roms for %s\n", name);
	return NULL;
    }

    if (read_disks(db, game.get()) < 0) {
        // TODO: use error reporting function
        printf("can't read disks for %s\n", name);
	return NULL;
    }

    return game;
}


static int
read_disks(romdb_t *db, Game *game) {
    sqlite3_stmt *stmt;
    int ret;
    disk_t *d;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_FILE)) == NULL)
	return -1;

    if (sqlite3_bind_int64(stmt, 1, game->id) != SQLITE_OK || sqlite3_bind_int(stmt, 2, TYPE_DISK) != SQLITE_OK)
	return -1;

    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        disk_t disk;
        disk_init(&disk);
        
	disk_name(&disk) = xstrdup(sq3_get_string(stmt, 0).c_str());
	disk_merge(&disk) = xstrdup(sq3_get_string(stmt, 1).c_str());
	disk.status = static_cast<status_t>(sqlite3_column_int(stmt, 2));
        disk.where = static_cast<where_t>(sqlite3_column_int(stmt, 3));
	sq3_get_hashes(disk_hashes(&disk), stmt, 5);
        
        game->disks.push_back(disk);
    }

    return (ret == SQLITE_DONE ? 0 : -1);
}


static int
read_roms(romdb_t *db, Game *game) {
    sqlite3_stmt *stmt;
    int ret;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_FILE)) == NULL) {
	return -1;
    }

    if (sqlite3_bind_int64(stmt, 1, game->id) != SQLITE_OK || sqlite3_bind_int(stmt, 2, TYPE_ROM) != SQLITE_OK) {
	return -1;
    }

    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        File rom;

        rom.name = sq3_get_string(stmt, 0);
	rom.merge = sq3_get_string(stmt, 1);
	rom.status = static_cast<status_t>(sqlite3_column_int(stmt, 2));
	rom.where = static_cast<where_t>(sqlite3_column_int(stmt, 3));
	rom.size = sq3_get_int64_default(stmt, 4, SIZE_UNKNOWN);
	sq3_get_hashes(&rom.hashes, stmt, 5);
        
        game->roms.push_back(rom);
    }

    return (ret == SQLITE_DONE ? 0 : -1);
}
