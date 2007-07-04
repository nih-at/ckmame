/*
  $NiH: r_file_by_hash.c,v 1.5 2006/04/15 22:52:58 dillo Exp $

  r_file_location.c -- read file_by_hash information from db
  Copyright (C) 2005-2007 Dieter Baron and Thomas Klausner

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



#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "dbh.h"
#include "file_location.h"
#include "sq_util.h"

const char *query_fbh[] = {
    "select g.name, f.file_idx from game g, file f" \
    " where f.game_id = g.game_id and f.file_type = ?"	\
    " and f.crc = ?",

    NULL,

    "select g.name, f.file_idx from game g, file f" \
    " where f.game_id = g.game_id and f.file_type = ?" \
    " and f.md5 = ?"
};

array_t *
r_file_by_hash(sqlite3 *db, filetype_t ft, const hashes_t *hash)
{
    sqlite3_stmt *stmt;
    array_t *a;
    file_location_t *fl;
    int ret;

    if (sqlite3_prepare_v2(db, query_fbh[ft], -1, &stmt, NULL) != SQLITE_OK)
	return NULL;

    if (sqlite3_bind_int(stmt, 1, ft) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return NULL;
    }
    switch (ft) {
    case TYPE_ROM:
	if (sqlite3_bind_int(stmt, 2, hashes_crc(hash)) != SQLITE_OK) {
	    sqlite3_finalize(stmt);
	    return NULL;
	}
	break;
    case TYPE_DISK:
	if (sqlite3_bind_blob(stmt, 2, hash->md5, HASHES_SIZE_MD5,
			      SQLITE_STATIC) != SQLITE_OK) {
	    sqlite3_finalize(stmt);
	    return NULL;
	}
	break;
    default:
	sqlite3_finalize(stmt);
	return NULL;
    }

    a = array_new(sizeof(file_location_t));

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	fl = array_grow(a, NULL);
	file_location_name(fl) = sq3_get_string(stmt, 0);
	file_location_index(fl) = sqlite3_column_int(stmt, 1);
    }

    sqlite3_finalize(stmt);

    if (ret != SQLITE_DONE) {
	array_free(a, file_location_finalize);
	return NULL;
    }

    return a;
}
