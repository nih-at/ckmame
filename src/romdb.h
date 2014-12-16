#ifndef _HAD_ROMDB_H
#define _HAD_ROMDB_H

/*
  romdb.h -- mame.db sqlite3 data base
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

#include "dbh.h"

typedef struct {
    dbh_t *dbh;
    int hashtypes[TYPE_MAX];
} romdb_t;

#define romdb_dbh(db)  ((db)->dbh)
#define romdb_sqlite3(db)	(dbh_db(romdb_dbh(db)))

int romdb_close(romdb_t *);
int romdb_delete_game(romdb_t *, const char *);
int romdb_has_disks(romdb_t *);
romdb_t *romdb_open(const char *, int);
dat_t *romdb_read_dat(romdb_t *);
detector_t *romdb_read_detector(romdb_t *);
array_t *romdb_read_file_by_hash(romdb_t *, filetype_t, const hashes_t *);
struct game *romdb_read_game(romdb_t *, const char *);
int romdb_hashtypes(romdb_t *, filetype_t);
parray_t *romdb_read_list(romdb_t *, enum dbh_list);
int romdb_update_game(romdb_t *, game_t *);
int romdb_update_game_parent(romdb_t *, game_t *, filetype_t);
int romdb_write_dat(romdb_t *, dat_t *);
int romdb_write_detector(romdb_t *db, const detector_t *);
int romdb_write_file_by_hash_parray(romdb_t *, filetype_t, const hashes_t *, parray_t *);
int romdb_write_game(romdb_t *, game_t *);
int romdb_write_hashtypes(romdb_t *, int, int);
int romdb_write_list(romdb_t *, const char *, const parray_t *);

#endif /* romdb.h */
