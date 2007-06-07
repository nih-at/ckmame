#ifndef _HAD_DBH_H
#define _HAD_DBH_H

/*
  $NiH: dbh.h,v 1.11 2006/10/04 17:36:43 dillo Exp $

  dbh.h -- mame.db sqlite3 data base
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

#include <sqlite3.h>

#include "dat.h"
#include "detector.h"
#include "file_location.h"
#include "game.h"
#include "hashes.h"
#include "parray.h"
#include "types.h"



#define DBH_FORMAT_VERSION	1 /* version of ckmame database format */

#define DBL_READ	0x0	/* open readonly */
#define DBL_WRITE	0x1	/* open for writing */
#define DBL_NEW		0x2	/* create new database */

/* keep in sync with r_list.c:query_list */
enum dbh_list {
    DBH_KEY_LIST_DISK,
    DBH_KEY_LIST_GAME,
    DBH_KEY_LIST_SAMPLE,
    DBH_KEY_LIST_MAX
};

#define DBH_DEFAULT_DB_NAME	"mame.db"
#define DBH_DEFAULT_OLD_DB_NAME	"old.db"

extern const char *sql_db_init;
extern const char *sql_db_init_2;



int dbh_close(sqlite3 *);
const char *dbh_error(sqlite3 *);
sqlite3* dbh_open(const char *, int);

int d_game(sqlite3 *, const char *);


dat_t *r_dat(sqlite3 *);
detector_t *r_detector(sqlite3 *);
array_t *r_file_by_hash(sqlite3 *, filetype_t, const hashes_t *);
struct game *r_game(sqlite3 *, const char *);
int r_hashtypes(sqlite3 *, int *, int *);
parray_t *r_list(sqlite3 *, enum dbh_list);
int u_game(sqlite3 *, game_t *);
int u_game_parent(sqlite3 *, game_t *, filetype_t);
int w_dat(sqlite3 *, dat_t *);
int w_detector(sqlite3 *db, const detector_t *);
int w_file_by_hash_parray(sqlite3 *, filetype_t, const hashes_t *, parray_t *);
int w_game(sqlite3 *, game_t *);
int w_hashtypes(sqlite3 *, int, int);
int w_list(sqlite3 *, const char *, const parray_t *);

#endif /* dbh.h */
