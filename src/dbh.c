/*
  dbh.c -- mame.db sqlite3 data base
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



#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "dbh.h"
#include "xmalloc.h"



#define DBH_ENOERR	0
#define DBH_EVERSION	2	/* version mismatch */
#define DBH_EMAX	3

#define QUERY_VERSION	"pragma user_version"
#define SET_VERSION_FMT	"pragma user_version = %d"

#define PRAGMAS		"PRAGMA synchronous = OFF; "

static int dbh_errno;

static int init_db(sqlite3 *);



static int
dbh_check_version(sqlite3 *db)
{
    sqlite3_stmt *stmt;
    int version;

    if (sqlite3_prepare_v2(db, QUERY_VERSION, -1, &stmt, NULL) != SQLITE_OK) {
	/* XXX */
	return -1;
    }
    if (sqlite3_step(stmt) != SQLITE_ROW) {
	sqlite3_finalize(stmt);
	/* XXX */
	return -1;
    }

    version = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    if (version != DBH_FORMAT_VERSION + 17000) {
	dbh_errno = DBH_EVERSION;
	return -1;
    }
    
    dbh_errno = DBH_ENOERR;
    return 0;
}



int
dbh_close(sqlite3 *db)
{
    return sqlite3_close(db);
}



const char *
dbh_error(sqlite3 *db)
{
    static const char *str[] = {
	"No error",
	"Database format version mismatch",
	"Unknown error"
    };

    /* XXX */
    if (dbh_errno == DBH_ENOERR)
	return sqlite3_errmsg(db);

    return str[dbh_errno<0||dbh_errno>DBH_EMAX ? DBH_EMAX : dbh_errno];
}



sqlite3 *
dbh_open(const char *name, int mode)
{
    sqlite3 *db;
    struct stat st;

    if (mode == DBL_NEW)
	unlink(name);
    else {
	if (stat(name, &st) != 0)
	    return NULL;
    }

    if (sqlite3_open(name, &db) != SQLITE_OK) {
	sqlite3_close(db);    
	return NULL;
    }

    if (sqlite3_exec(db, PRAGMAS, NULL, NULL, NULL) != SQLITE_OK) {
	sqlite3_close(db);    
	return NULL;
    }

    if (mode == DBL_NEW) {
	if (init_db(db) < 0) {
	    sqlite3_close(db);
	    unlink(name);
	    return NULL;
	}
    }
    else if (dbh_check_version(db) != 0) {
	sqlite3_close(db);
	return NULL;
    }

    return db;
}



static int
init_db(sqlite3 *db)
{
    char b[256];

    sprintf(b, SET_VERSION_FMT, DBH_FORMAT_VERSION + 17000);
    if (sqlite3_exec(db, b, NULL, NULL, NULL) != SQLITE_OK)
	return -1;

    if (sqlite3_exec(db, sql_db_init, NULL, NULL, NULL) != SQLITE_OK)
	return -1;

    return 0;
}
