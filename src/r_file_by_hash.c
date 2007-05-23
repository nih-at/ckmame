/*
  $NiH: r_file_by_hash.c,v 1.5 2006/04/15 22:52:58 dillo Exp $

  r_file_location.c -- read file_by_hash information from db
  Copyright (C) 2005-2006 Dieter Baron and Thomas Klausner

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
