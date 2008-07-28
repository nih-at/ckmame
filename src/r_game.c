/*
  r_game.c -- read game struct from db
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



/* read struct game from db */

#include <string.h>
#include <stdlib.h>

#include "dbh.h"
#include "game.h"
#include "sq_util.h"
#include "xmalloc.h"

#define QUERY_GAME	\
	"select game_id, description, dat_idx from game where name = ?"
#define QUERY_PARENT	\
	"select parent from parent where game_id = ? and file_type = ?"
#define QUERY_GPARENT	\
	"select parent from parent p, game g " \
	"where g.game_id = p.game_id and g.name = ? and p.file_type = ?"
#define QUERY_FILE	\
	"select name, merge, status, location, size, crc, md5, sha1 " \
	"from file where game_id = ? and file_type = ? order by file_idx"

static int read_disks(sqlite3 *, game_t *);
static int read_rs(sqlite3 *, game_t *, filetype_t);
static void sq3_get_hashes(hashes_t *, sqlite3_stmt *, int);



game_t *
r_game(sqlite3 *db, const char *name)
{
    sqlite3_stmt *stmt;
    game_t *game;
    int i;

    if (sqlite3_prepare_v2(db, QUERY_GAME, -1, &stmt, NULL) != SQLITE_OK)
	return NULL;

    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_ROW) {
	sqlite3_finalize(stmt);
	return NULL;
    }

    game = game_new();
    game->id = sqlite3_column_int(stmt, 0);
    game->name = xstrdup(name);
    game->description = sq3_get_string(stmt, 1);
    game->dat_no = sqlite3_column_int(stmt, 2);

    sqlite3_finalize(stmt);

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
read_disks(sqlite3 *db, game_t *g)
{
    sqlite3_stmt *stmt;
    int ret;
    disk_t *d;

    if (sqlite3_prepare_v2(db, QUERY_FILE, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    if (sqlite3_bind_int(stmt, 1, game_id(g)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, TYPE_DISK) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return -1;
    }

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	d = (disk_t *)array_grow(game_disks(g), disk_init);

	disk_name(d) = sq3_get_string(stmt, 0);
	disk_merge(d) = sq3_get_string(stmt, 1);
	disk_status(d) = sqlite3_column_int(stmt, 2);
	sq3_get_hashes(disk_hashes(d), stmt, 5);
    }

    sqlite3_finalize(stmt);

    return (ret == SQLITE_DONE ? 0 : -1);
}



static int
read_rs(sqlite3 *db, game_t *g, filetype_t ft)
{
    sqlite3_stmt *stmt;
    int ret;
    file_t *r;

    if (sqlite3_prepare_v2(db, QUERY_PARENT, -1, &stmt, NULL) != SQLITE_OK)
	return -1;
    if (sqlite3_bind_int(stmt, 1, game_id(g)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return -1;
    }
    if ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	game_cloneof(g, ft, 0) = sq3_get_string(stmt, 0);
    }
    sqlite3_finalize(stmt);
    if (ret != SQLITE_ROW && ret != SQLITE_DONE)
	return -1;

    if (game_cloneof(g, ft, 0)) {
	if (sqlite3_prepare_v2(db, QUERY_GPARENT, -1, &stmt, NULL)
	    != SQLITE_OK)
	    return -1;
	if (sq3_set_string(stmt, 1, game_cloneof(g, ft, 0)) != SQLITE_OK
	    || sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK) {
	    sqlite3_finalize(stmt);
	    return -1;
	}
	if ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	    game_cloneof(g, ft, 1) = sq3_get_string(stmt, 0);
	}
	sqlite3_finalize(stmt);
	
	if (ret != SQLITE_ROW && ret != SQLITE_DONE)
	    return -1;
    }

    if (sqlite3_prepare_v2(db, QUERY_FILE, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    if (sqlite3_bind_int(stmt, 1, game_id(g)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, ft) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return -1;
    }

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	r = (file_t *)array_grow(game_files(g, ft), file_init);

	file_name(r) = sq3_get_string(stmt, 0);
	file_merge(r) = sq3_get_string(stmt, 1);
	file_status(r) = sqlite3_column_int(stmt, 2);
	file_where(r) = sqlite3_column_int(stmt, 3);
	file_size(r) = sq3_get_int64_default(stmt, 4, SIZE_UNKNOWN);
	sq3_get_hashes(file_hashes(r), stmt, 5);
    }

    sqlite3_finalize(stmt);

    return (ret == SQLITE_DONE ? 0 : -1);
}



static
void sq3_get_hashes(hashes_t *h, sqlite3_stmt *stmt, int col)
{
    if (sqlite3_column_type(stmt, col) != SQLITE_NULL) {
	hashes_crc(h) = sqlite3_column_int64(stmt, col) & 0xffffffff;
	hashes_types(h) |= HASHES_TYPE_CRC;
    }
    if (sqlite3_column_type(stmt, col+1) != SQLITE_NULL) {
	memcpy(h->md5, sqlite3_column_blob(stmt, col+1),
	       HASHES_SIZE_MD5);
	hashes_types(h) |= HASHES_TYPE_MD5;
    }
    if (sqlite3_column_type(stmt, col+2) != SQLITE_NULL) {
	memcpy(h->sha1, sqlite3_column_blob(stmt, col+2),
	       HASHES_SIZE_SHA1);
	hashes_types(h) |= HASHES_TYPE_SHA1;
    }
}
