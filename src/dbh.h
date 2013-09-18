#ifndef _HAD_DBH_H
#define _HAD_DBH_H

/*
  dbh.h -- mame.db sqlite3 data base
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

#include <sqlite3.h>

#include "dat.h"
#include "dbh_statements.h"
#include "detector.h"
#include "file_location.h"
#include "game.h"
#include "hashes.h"
#include "parray.h"
#include "types.h"



#define DBH_FMT_MAME	0x0	/* mame.db format */
#define DBH_FMT_MEM	0x1	/* in-memory db format */
#define DBH_FMT_DIR	0x2	/* unpacked dirs db format */
#define DBH_FMT(m)	((m) & 0xf)

#define DBH_READ	0x00	/* open readonly */
#define DBH_WRITE	0x10	/* open for writing */
#define DBH_NEW		0x20	/* create new database */
#define DBH_FLAGS(m)	((m) & 0xf0)

/* keep in sync with romdb_read_list.c:query_list */
enum dbh_list {
    DBH_KEY_LIST_DISK,
    DBH_KEY_LIST_GAME,
    DBH_KEY_LIST_SAMPLE,
    DBH_KEY_LIST_MAX
};

#define DBH_DEFAULT_DB_NAME	"mame.db"
#define DBH_DEFAULT_OLD_DB_NAME	"old.db"

extern const char *sql_db_init[];
extern const char *sql_db_init_2;

struct dbh {
    sqlite3 *db;
    sqlite3_stmt *statements[DBH_STMT_MAX];
    int dbh_errno;
    int format;
};
typedef struct dbh dbh_t;

typedef dbh_t romdb_t;

#define dbh_db(dbh)	((dbh)->db)

int dbh_close(dbh_t *);
const char *dbh_error(dbh_t *);
dbh_t* dbh_open(const char *, int);

sqlite3_stmt *dbh_get_statement(dbh_t *, dbh_stmt_t);
dbh_stmt_t dbh_stmt_with_hashes_and_size(dbh_stmt_t, const hashes_t *, int);

int romdb_delete_game(romdb_t *, const char *);


dat_t *romdb_read_dat(romdb_t *);
detector_t *romdb_read_detector(romdb_t *);
array_t *romdb_read_file_by_hash(romdb_t *, filetype_t, const hashes_t *);
struct game *romdb_read_game(romdb_t *, const char *);
int romdb_read_hashtypes(romdb_t *, int *, int *);
parray_t *romdb_read_list(romdb_t *, enum dbh_list);
int romdb_update_game(romdb_t *, game_t *);
int romdb_update_game_parent(romdb_t *, game_t *, filetype_t);
int romdb_write_dat(romdb_t *, dat_t *);
int romdb_write_detector(romdb_t *db, const detector_t *);
int romdb_write_file_by_hash_parray(romdb_t *, filetype_t, const hashes_t *, parray_t *);
int romdb_write_game(romdb_t *, game_t *);
int romdb_write_hashtypes(romdb_t *, int, int);
int romdb_write_list(romdb_t *, const char *, const parray_t *);

#endif /* dbh.h */
