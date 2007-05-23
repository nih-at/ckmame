/*
  $NiH: w_game.c,v 1.6 2006/04/15 22:52:58 dillo Exp $

  w_game.c -- write game struct to db
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

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



/* write struct game to db */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "dbh.h"
#include "game.h"
#include "sq_util.h"
#include "util.h"

#define QUERY_GAME_ID	"select game_id from game where name = ?"
#define DELETE_GAME	"delete from game where game_id = ?"
#define DELETE_FILE	"delete from file where game_id = ?"
#define DELETE_PARENT	"delete from parent where game_id = ?"
#define DELETE_PARENT_FT	\
    "delete from parent where game_id = ? and file_type = ?"

#define INSERT_GAME	"insert into game (name, description, dat_idx) " \
			"values (?, ?, ?)"
#define INSERT_PARENT	"insert into parent (game_id, file_type, parent) " \
			"values (?, ?, ?)"
#define INSERT_FILE	\
	"insert into file (game_id, file_type, file_idx, name, merge, " \
	"status, location, size, crc, md5, sha1) values " \
	"(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"

#define UPDATE_FILE	\
	"update file set location = ? where game_id = ? and file_type = ? " \
 	"and file_idx = ?"
#define UPDATE_PARENT	\
    "update parent set parent = ? where game_id = ? and file_type = ?"

static int sq3_set_hashes(const hashes_t *, sqlite3_stmt *, int);
static int write_disks(sqlite3 *, const game_t *);
static int write_rs(sqlite3 *, const game_t *, filetype_t);



int
d_game(sqlite3 *db, const char *name)
{
    sqlite3_stmt *stmt;
    int ret, id;

    if (sqlite3_prepare_v2(db, QUERY_GAME_ID, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return -1;
    }

    if ((ret=sqlite3_step(stmt)) == SQLITE_ROW)
	id = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    if (ret == SQLITE_DONE)
	return 0;
    else if (ret != SQLITE_ROW)
	return -1;

    ret = 0;

    if (sqlite3_prepare_v2(db, DELETE_GAME, -1, &stmt, NULL) != SQLITE_OK)
	return -1;
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_DONE)
	ret = -1;
    sqlite3_finalize(stmt);

    if (sqlite3_prepare_v2(db, DELETE_FILE, -1, &stmt, NULL) != SQLITE_OK)
	return -1;
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_DONE)
	ret = -1;
    sqlite3_finalize(stmt);

    if (sqlite3_prepare_v2(db, DELETE_PARENT, -1, &stmt, NULL) != SQLITE_OK)
	return -1;
    if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_DONE)
	ret = -1;
    sqlite3_finalize(stmt);

    return ret;
}



int
u_game(sqlite3 *db, game_t *g)
{
    sqlite3_stmt *stmt;
    int ft, i;
    rom_t *r;

    if (sqlite3_prepare_v2(db, UPDATE_FILE, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    if (sqlite3_bind_int(stmt, 2, game_id(g)) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return -1;
    }

    for (ft=0; ft<GAME_RS_MAX; ft++) {
	if (sqlite3_bind_int(stmt, 3, ft) != SQLITE_OK) {
	    sqlite3_finalize(stmt);
	    return -1;
	}
	for (i=0; i<game_num_files(g, ft); i++) {
	    r = game_file(g, ft, i);
	    if (rom_where(r) == ROM_INZIP)
		continue;

	    if (sqlite3_bind_int(stmt, 1, rom_where(r)) != SQLITE_OK
		|| sqlite3_bind_int(stmt, 4, i) != SQLITE_OK
		|| sqlite3_step(stmt) != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		return -1;
	    }
	}
    }

    return 0;
}



int
u_game_parent(sqlite3 *db, game_t *g, filetype_t ft)
{
    sqlite3_stmt *stmt;
    const char *query;
    int off;

    if (game_cloneof(g, ft, 0)) {
	query = UPDATE_PARENT;
	off = 2;
    }
    else {
	query = DELETE_PARENT_FT;
	off = 1;
    }

    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    if (sqlite3_bind_int(stmt, off, game_id(g)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, off+1, ft) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return -1;
    }
    if (game_cloneof(g, ft, 0)
	&& sqlite3_bind_text(stmt, 1, game_cloneof(g, ft, 0),
			     -1, SQLITE_STATIC) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return -1;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE
	|| sqlite3_finalize(stmt) != SQLITE_OK)
	return -1;

    return 0;
}




int
w_game(sqlite3 *db, game_t *g)
{
    sqlite3_stmt *stmt;
    int i;

    d_game(db, game_name(g));

    if (sqlite3_prepare_v2(db, INSERT_GAME, -1, &stmt, NULL) != SQLITE_OK)
	return -1;
    if (sq3_set_string(stmt, 1, game_name(g)) < 0
	|| sq3_set_string(stmt, 2, game_description(g)) < 0
	|| sqlite3_bind_int(stmt, 3, game_dat_no(g)) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_DONE) {
	sqlite3_finalize(stmt);
	return -1;
    }

    game_id(g) = sqlite3_last_insert_rowid(db);

    sqlite3_finalize(stmt);

    if (write_disks(db, g) < 0) {
	d_game(db, game_name(g));
	return -1;
    }


    for (i=0; i<GAME_RS_MAX; i++) {
	if (write_rs(db, g, i) < 0) {
	    d_game(db, game_name(g));
	    return -1;
	}
    }

    return 0;
}



static int
sq3_set_hashes(const hashes_t *h, sqlite3_stmt *stmt, int col)
{
    int ret;
    
    if (hashes_has_type(h, HASHES_TYPE_CRC))
	ret = sqlite3_bind_int(stmt, col, hashes_crc(h));
    else
	ret = sqlite3_bind_null(stmt, col);
    if (ret != SQLITE_OK)
	return -1;

    if (hashes_has_type(h, HASHES_TYPE_MD5))
	ret = sqlite3_bind_blob(stmt, col+1, h->md5, HASHES_SIZE_MD5,
				SQLITE_STATIC);
    else
	ret = sqlite3_bind_null(stmt, col+1);
    if (ret != SQLITE_OK)
	return -1;

    if (hashes_has_type(h, HASHES_TYPE_SHA1))
	ret = sqlite3_bind_blob(stmt, col+2, h->sha1, HASHES_SIZE_SHA1,
				SQLITE_STATIC);
    else
	ret = sqlite3_bind_null(stmt, col+2);
    if (ret != SQLITE_OK)
	return -1;

    return 0;
}



static int
write_disks(sqlite3 *db, const game_t *g)
{
    sqlite3_stmt *stmt;
    int i;
    disk_t *d;

    if (game_num_disks(g) == 0)
	return 0;
    
    if (sqlite3_prepare_v2(db, INSERT_FILE, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    if (sqlite3_bind_int(stmt, 1, game_id(g)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, TYPE_DISK) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 7, 0) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return -1;
    }

    for (i=0; i<game_num_disks(g); i++) {
	d = game_disk(g, i);

	if (sqlite3_bind_int(stmt, 3, i) != SQLITE_OK
	    || sq3_set_string(stmt, 4, disk_name(d)) < 0
	    || sq3_set_string(stmt, 5, disk_merge(d)) < 0
	    || sqlite3_bind_int(stmt, 6, disk_status(d)) != SQLITE_OK
	    || sq3_set_hashes(disk_hashes(d), stmt, 9) < 0
	    || sqlite3_step(stmt) != SQLITE_DONE
	    || sqlite3_reset(stmt) != SQLITE_OK) {
	    sqlite3_finalize(stmt);
	    return -1;
	}
    }

    sqlite3_finalize(stmt);

    return 0;
}



static int
write_rs(sqlite3 *db, const game_t *g, filetype_t ft)
{
    sqlite3_stmt *stmt;
    int i;
    rom_t *r;

    if (game_cloneof(g, ft, 0)) {
	if (sqlite3_prepare_v2(db, INSERT_PARENT, -1, &stmt, NULL)
	    != SQLITE_OK)
	    return -1;

	if (sqlite3_bind_int(stmt, 1, game_id(g)) != SQLITE_OK
	    || sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK
	    || sq3_set_string(stmt, 3, game_cloneof(g, ft, 0)) < 0
	    || sqlite3_step(stmt) != SQLITE_DONE) {
	    sqlite3_finalize(stmt);
	    return -1;
	}

	sqlite3_finalize(stmt);
    }

    if (game_num_files(g, ft) == 0)
	return 0;

    if (sqlite3_prepare_v2(db, INSERT_FILE, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    if (sqlite3_bind_int(stmt, 1, game_id(g)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return -1;
    }

    for (i=0; i<game_num_files(g, ft); i++) {
	r = game_file(g, ft, i);

	if (sqlite3_bind_int(stmt, 3, i) != SQLITE_OK
	    || sq3_set_string(stmt, 4, rom_name(r)) < 0
	    || sq3_set_string(stmt, 5, rom_merge(r)) < 0
	    || sqlite3_bind_int(stmt, 6, rom_status(r)) != SQLITE_OK
	    || sqlite3_bind_int(stmt, 7, rom_where(r)) != SQLITE_OK
	    || sq3_set_int64_default(stmt, 8, rom_size(r),
				     SIZE_UNKNOWN) != SQLITE_OK
	    || sq3_set_hashes(rom_hashes(r), stmt, 9) < 0
	    || sqlite3_step(stmt) != SQLITE_DONE
	    || sqlite3_reset(stmt) != SQLITE_OK) {
	    sqlite3_finalize(stmt);
	    return -1;
	}
    }

    sqlite3_finalize(stmt);

    return 0;
}
