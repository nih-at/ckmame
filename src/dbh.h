#ifndef _HAD_DBH_H
#define _HAD_DBH_H

/*
  $NiH: dbh.h,v 1.11 2006/10/04 17:36:43 dillo Exp $

  dbh.h -- mame.db sqlite3 data base
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
