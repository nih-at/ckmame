/*
  $NiH: r_list.c,v 1.5 2006/04/15 22:52:58 dillo Exp $

  r_list.c -- read list struct from db
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



/* read list of strings from db */
#include <stdlib.h>
#include <string.h>

#include "dbh.h"
#include "sq_util.h"
#include "xmalloc.h"

#define QUERY_LIST_GAME	"select name from game"

#define QUERY_HASH_TYPE	"select name from file " \
			"where file_type = %d and %s not null limit 1"

static void r__hashtypes_ft(sqlite3 *, filetype_t, int *);



int
r_hashtypes(sqlite3 *db, int *romhashtypesp, int *diskhashtypesp)
{
    r__hashtypes_ft(db, TYPE_ROM, romhashtypesp);
    r__hashtypes_ft(db, TYPE_DISK, diskhashtypesp);

    return 0;
}



parray_t *
r_list(sqlite3 *db, const char *key)
{
    parray_t *pa;
    sqlite3_stmt *stmt;
    int ret;
    
    if (strcmp(key, DBH_KEY_LIST_GAME) != 0)
	return NULL;

    if (sqlite3_prepare_v2(db, QUERY_LIST_GAME,
			   -1, &stmt, NULL) != SQLITE_OK) {
	/* XXX */
	return NULL;
    }

    pa = parray_new();

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW)
	parray_push(pa, sq3_get_string(stmt, 0));

    sqlite3_finalize(stmt);

    if (ret != SQLITE_DONE) {
	/* XXX */
	parray_free(pa, free);
	return NULL;
    }
    
    return pa;
}



static void
r__hashtypes_ft(sqlite3 *db, filetype_t ft, int *typesp)
{
    char buf[256];
    int type;
    sqlite3_stmt *stmt;

    *typesp = 0;

    for (type=1; type<=HASHES_TYPE_MAX; type<<=1) {
	sprintf(buf, QUERY_HASH_TYPE,
		ft, hash_type_string(type));
	if (sqlite3_prepare_v2(db, buf, -1, &stmt, NULL) != SQLITE_OK)
	    continue;
	if (sqlite3_step(stmt) == SQLITE_ROW)
	    *typesp |= type;
	sqlite3_finalize(stmt);
    }
}
