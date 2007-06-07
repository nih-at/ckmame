/*
  $NiH: dbdump.c,v 1.8 2006/10/04 18:22:57 dillo Exp $

  dbdump.c -- print contents of db
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



#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include <sqlite3.h>

#include "error.h"
#include "util.h"

#define QUERY_TABLES	\
    "select name from sqlite_master where type='table'" \
    " and name not like 'sqlite_%' order by name"

#define QUERY_COLS_FMT	"pragma table_info(%s)"

const char *usage = "usage: %s db-file\n";

static int dump_db(sqlite3 *);
static int dump_table(sqlite3 *, const char *);



int
main(int argc, char *argv[])
{
    sqlite3 *db;
    char *fname;
    struct stat st;
    int ret;

    setprogname(argv[0]);

    if (argc != 2) {
	fprintf(stderr, usage, getprogname());
	exit(1);
    }

    fname = argv[1];

    seterrinfo(fname, NULL);

    if (stat(fname, &st) != 0) {
	myerror(ERRSTR, "can't stat database `%s'", fname);
	exit(1);
    }

    if (sqlite3_open(fname, &db) != SQLITE_OK) {
	seterrdb(db);
	myerror(ERRDB, "can't open database `%s'", fname);
	sqlite3_close(db);
	exit(1);
    }

    seterrdb(db);
    
    if ((ret=dump_db(db)) < 0)
	myerror(ERRDB, "can't dump database `%s'", fname);

    sqlite3_close(db);

    return ret < 0 ? 1 : 0;
}



static int
dump_db(sqlite3 *db)
{
    sqlite3_stmt *stmt;
    int ret;

    if (sqlite3_prepare_v2(db, QUERY_TABLES, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	if (dump_table(db, (const char *)sqlite3_column_text(stmt, 0)) < 0)
	    break;
    }

    sqlite3_finalize(stmt);

    if (ret != SQLITE_DONE)
	return -1;

    return 0;
}



static int
dump_table(sqlite3 *db, const char *tbl)
{
    sqlite3_stmt *stmt;
    char b[8192];
    int first_col, first_key;
    int i, ret;

    sprintf(b, QUERY_COLS_FMT, tbl);

    if (sqlite3_prepare_v2(db, b, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    printf(">>> table %s (", tbl);
    sprintf(b, "select * from %s", tbl);
    first_col = first_key = 1;

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	printf("%s%s",
	       first_col ? "" : ", ",
	       sqlite3_column_text(stmt, 1));
	first_col = 0;

	if (sqlite3_column_int(stmt, 5)) {
	    sprintf(b+strlen(b), "%s%s",
		    first_key ? " order by " : ", ",
		    sqlite3_column_text(stmt, 1));
	    first_key = 0;
	}
    }

    printf(")\n");
    sqlite3_finalize(stmt);

    if (ret != SQLITE_DONE)
	return -1;

    if (sqlite3_prepare_v2(db, b, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	for (i=0; i<sqlite3_column_count(stmt); i++) {
	    if (i > 0)
		printf("|");

	    switch (sqlite3_column_type(stmt, i)) {
	    case SQLITE_INTEGER:
	    case SQLITE_FLOAT:
	    case SQLITE_TEXT:
		printf("%s", sqlite3_column_text(stmt, i));
		break;
	    case SQLITE_NULL:
		printf("<null>");
		break;
	    case SQLITE_BLOB:
		bin2hex(b, sqlite3_column_blob(stmt, i),
			sqlite3_column_bytes(stmt, i));
		printf("<%s>", b);
		break;
	    }
	}
	printf("\n");
    }

    sqlite3_finalize(stmt);

    if (ret != SQLITE_DONE)
	return -1;

    return 0;
}
