/*
  romdb_read_list.c -- read list struct from db
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



/* read list of strings from db */
#include <stdlib.h>
#include <string.h>

#include "romdb.h"
#include "sq_util.h"
#include "xmalloc.h"

/* keep in sync with dbh.h:enum list */
const dbh_stmt_t query_list[] = {
    DBH_STMT_QUERY_LIST_DISK,
    DBH_STMT_QUERY_LIST_GAME,
    DBH_STMT_QUERY_LIST_SAMPLE
};

const dbh_stmt_t query_hash_type[] = {
    DBH_STMT_QUERY_HASH_TYPE_CRC,
    DBH_STMT_QUERY_HASH_TYPE_MD5,
    DBH_STMT_QUERY_HASH_TYPE_SHA1
};

static void romdb_read__hashtypes_ft(romdb_t *, filetype_t, int *);



int
romdb_read_hashtypes(romdb_t *db, int *romhashtypesp, int *diskhashtypesp)
{
    romdb_read__hashtypes_ft(db, TYPE_ROM, romhashtypesp);
    romdb_read__hashtypes_ft(db, TYPE_DISK, diskhashtypesp);

    return 0;
}



parray_t *
romdb_read_list(romdb_t *db, enum dbh_list type)
{
    parray_t *pa;
    sqlite3_stmt *stmt;
    int ret;

    if (type >= DBH_KEY_LIST_MAX)
	return NULL;

    if ((stmt = dbh_get_statement(romdb_dbh(db), query_list[type])) == NULL)
	return NULL;

    pa = parray_new();

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW)
	parray_push(pa, sq3_get_string(stmt, 0));

    if (ret != SQLITE_DONE) {
	parray_free(pa, free);
	return NULL;
    }
    
    return pa;
}



static void
romdb_read__hashtypes_ft(romdb_t *db, filetype_t ft, int *typesp)
{
    int type;
    sqlite3_stmt *stmt;

    *typesp = 0;

    for (type=0; (1<<type)<=HASHES_TYPE_MAX; type++) {
	if ((stmt = dbh_get_statement(romdb_dbh(db), query_hash_type[type])) == NULL)
	    continue;
	if (sqlite3_bind_int(stmt, 1, ft) != SQLITE_OK)
	    continue;
	if (sqlite3_step(stmt) == SQLITE_ROW)
	    *typesp |= (1<<type);
    }
}
