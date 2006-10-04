#ifndef _HAD_DBH_H
#define _HAD_DBH_H

/*
  $NiH: dbh.h,v 1.10 2006/09/29 16:01:33 dillo Exp $

  dbh.h -- compressed on-disk mame.db data base
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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

#include "dat.h"
#include "dbl.h"
#include "file_location.h"
#include "game.h"
#include "hashes.h"
#include "parray.h"
#include "types.h"



#define DBH_FORMAT_VERSION	13 /* version of ckmame database format */

#define DBH_KEY_DAT		"/dat"
#define DBH_KEY_DB_VERSION	"/ckmame"
#define DBH_KEY_HASH_TYPES	"/hashtypes"
#define DBH_KEY_LIST_DISK	"/list/disk"
#define DBH_KEY_LIST_GAME	"/list/game"
#define DBH_KEY_LIST_SAMPLE	"/list/sample"

#define DBH_DEFAULT_DB_NAME	"mame.db"
#define DBH_DEFAULT_OLD_DB_NAME	"old.db"



int dbh_check_version(DB *, int);	/* XXX: really part of API? */
int dbh_close(DB *);
const char *dbh_error(void);
int dbh_insert(DB *, const char *, const DBT *);
int dbh_lookup(DB *, const char *, DBT *);
DB* dbh_open(const char *, int);

dat_t *r_dat(DB *);
array_t *r_file_by_hash(DB *, filetype_t, const hashes_t *);
struct game *r_game(DB *, const char *);
int r_hashtypes(DB *, int *, int *);
parray_t *r_list(DB *, const char *);
int w_dat(DB *, dat_t *);
int w_file_by_hash_parray(DB *, filetype_t, const hashes_t *, parray_t *);
int w_game(DB *, const game_t *);
int w_hashtypes(DB *, int, int);
int w_list(DB *, const char *, const parray_t *);

const char *filetype_db_key(filetype_t);

#endif /* dbh.h */
