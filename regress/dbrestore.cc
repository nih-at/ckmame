/*
 dbdump.c -- restore db from dump
 Copyright (C) 2014 Dieter Baron and Thomas Klausner

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


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <sqlite3.h>

#include "compat.h"
#include "dbh.h"
#include "error.h"
#include "parray.h"
#include "sq_util.h"
#include "util.h"
#include "xmalloc.h"

static int column_type(const char *name);
static int db_type(const char *name);
static char *get_line(FILE *f);
static int restore_db(dbh_t *dbh, FILE *f);
static int restore_table(dbh_t *dbh, FILE *f);
static void unget_line(char *line);


#define QUERY_COLS_FMT "pragma table_info(%s)"

const char *usage = "usage: %s [-t db-type] dump-file db-file\n";

char help_head[] = PACKAGE " by Dieter Baron and Thomas Klausner\n\n";

char help[] = "\n"
	      "  -h, --help              display this help message\n"
	      "  -t, --type TYPE         restore database of type TYPE (ckmamedb, mamedb, memdb)"
	      "  -V, --version           display version number\n"
	      "\nReport bugs to " PACKAGE_BUGREPORT ".\n";

char version_string[] = PACKAGE " " VERSION "\n"
				"Copyright (C) 2014 Dieter Baron and Thomas Klausner\n" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";


#define OPTIONS "ht:V"
struct option options[] = {{"help", 0, 0, 'h'},
			   {"version", 0, 0, 'V'},

			   {"type", 1, 0, 't'}};


int
main(int argc, char *argv[]) {
    setprogname(argv[0]);

    int type = DBH_FMT_MAME;

    opterr = 0;
    int c;
    while ((c = getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
	switch (c) {
	case 'h':
	    fputs(help_head, stdout);
	    printf(usage, getprogname());
	    fputs(help, stdout);
	    exit(0);
	case 'V':
	    fputs(version_string, stdout);
	    exit(0);

	case 't':
	    if ((type = db_type(optarg)) < 0) {
		fprintf(stderr, "%s: unknown db type %s\n", getprogname(), optarg);
		fprintf(stderr, usage, getprogname());
		exit(1);
	    }
	    break;
	}
    }

    if (optind != argc - 2) {
	fprintf(stderr, usage, getprogname());
	exit(1);
    }

    const char *dump_fname = argv[optind];
    const char *db_fname = argv[optind + 1];

    seterrinfo(dump_fname, "");

    FILE *f = fopen(dump_fname, "r");
    if (f == NULL) {
	myerror(ERRSTR, "can't open dump '%s'", dump_fname);
	exit(1);
    }

    dbh_t *dbh = dbh_open(db_fname, type | DBH_TRUNCATE | DBH_WRITE | DBH_CREATE);
    if (dbh == NULL) {
	myerror(ERRDB, "can't create database '%s'", db_fname);
	exit(1);
    }

    seterrdb(dbh);

    if (restore_db(dbh, f) < 0)
	exit(1);

    if (type == DBH_FMT_MAME) {
	if (sqlite3_exec(dbh_db(dbh), sql_db_init_2, NULL, NULL, NULL) != SQLITE_OK) {
	    myerror(ERRDB, "can't create indices");
	    exit(1);
	}
    }

    dbh_close(dbh);

    exit(0);
}


static int
column_type(const char *name) {
    if (strcmp(name, "integer") == 0)
	return SQLITE_INTEGER;
    if (strcmp(name, "text") == 0)
	return SQLITE_TEXT;
    if (strcmp(name, "binary") == 0 || strcmp(name, "blob") == 0)
	return SQLITE_BLOB;

    return -1;
}


static int
db_type(const char *name) {
    if (strcmp(name, "mamedb") == 0)
	return DBH_FMT_MAME;
    if (strcmp(name, "memdb") == 0)
	return DBH_FMT_MEM;
    if (strcmp(name, "ckmamedb") == 0)
	return DBH_FMT_DIR;

    return -1;
}


static char *ungot_line;

static char *
get_line(FILE *f) {
    static char *buffer = NULL;
    static size_t buffer_size = 0;

    if (ungot_line) {
	char *line = ungot_line;
	ungot_line = NULL;
	return line;
    }

    if (getline(&buffer, &buffer_size, f) < 0)
	return NULL;

    if (buffer[strlen(buffer) - 1] == '\n')
	buffer[strlen(buffer) - 1] = '\0';
    return buffer;
}

static int
restore_db(dbh_t *dbh, FILE *f) {
    ungot_line = NULL;

    while (!feof(f)) {
	if (restore_table(dbh, f) < 0)
	    return -1;

	if (ferror(f)) {
	    myerror(ERRFILESTR, "read error");
	    return -1;
	}
    }

    return 0;
}


static int
restore_table(dbh_t *dbh, FILE *f) {
    char query[8192];

    char *line = get_line(f);

    if (line == NULL)
	return 0;

    if (strncmp(line, ">>> table ", 10) != 0 || line[strlen(line) - 1] != ')') {
	myerror(ERRFILE, "invalid format of table header");
	return -1;
    }

    char *table_name = line + 10;
    char *p = strchr(table_name, ' ');
    if (p == NULL || p[1] != '(') {
	myerror(ERRFILE, "invalid format of table header");
	return -1;
    }
    *p = 0;

    char *columns = p + 2;

    sprintf(query, QUERY_COLS_FMT, table_name);
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(dbh_db(dbh), query, -1, &stmt, NULL) != SQLITE_OK) {
	myerror(ERRDB, "can't query table %s", table_name);
	return -1;
    }

    array_t *column_types = array_new(sizeof(int));
    int ret;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
	if (columns == NULL) {
	    myerror(ERRFILE, "too few columns in dump for table %s", table_name);
	    array_free(column_types, NULL);
	    return -1;
	}
	char *column = columns;
	p = column + strcspn(column, ", )");
	if (*p == ',')
	    columns = p + 1 + strspn(p + 1, " ");
	else if (*p == ')')
	    columns = NULL;
	else {
	    myerror(ERRFILE, "invalid format of table header");
	    array_free(column_types, NULL);
	    return -1;
	}
	*p = '\0';

	if (strcmp(column, (const char *)sqlite3_column_text(stmt, 1)) != 0) {
	    myerror(ERRFILE, "column %s in dump doesn't match column in db %s for table %s", column, sqlite3_column_text(stmt, 1), table_name);
	    array_free(column_types, NULL);
	    return -1;
	}

	int coltype = column_type((const char *)sqlite3_column_text(stmt, 2));
	if (coltype < 0) {
	    myerror(ERRDB, "unsupported column type %s for column %s of table %s", sqlite3_column_text(stmt, 1), column, table_name);
	    array_free(column_types, NULL);
	    return -1;
	}
	array_push(column_types, &coltype);
    }

    sqlite3_finalize(stmt);

    sprintf(query, "insert into %s values (", table_name);
    p = query + strlen(query);
    int i;
    for (i = 0; i < array_length(column_types); i++) {
	sprintf(p, "%s?", i == 0 ? "" : ", ");
	p += strlen(p);
    }
    sprintf(p, ")");

    if (sqlite3_prepare_v2(dbh_db(dbh), query, -1, &stmt, NULL) != SQLITE_OK) {
	myerror(ERRDB, "can't prepare insert statement for table %s", table_name);
	array_free(column_types, NULL);
	return -1;
    }

    while ((line = get_line(f))) {
	if (strncmp(line, ">>> table ", 10) == 0) {
	    unget_line(line);
	    break;
	}

	for (i = 0; i < array_length(column_types); i++) {
	    char *value = strtok(i == 0 ? line : NULL, "|");

	    if (value == NULL) {
		myerror(ERRFILE, "too few columns in row for table %s", table_name);
		array_free(column_types, NULL);
		return -1;
	    }

	    if (strcmp(value, "<null>") == 0)
		ret = sqlite3_bind_null(stmt, i + 1);
	    else {
		switch (*(int *)array_get(column_types, i)) {
		case SQLITE_TEXT:
		    ret = sq3_set_string(stmt, i + 1, value);
		    break;

		case SQLITE_INTEGER: {
		    intmax_t vi = strtoimax(value, NULL, 10);
		    ret = sqlite3_bind_int64(stmt, i + 1, vi);
		    break;
		}

		case SQLITE_BLOB: {
		    size_t length = strlen(value);

		    if (value[0] != '<' || value[length - 1] != '>' || length % 2) {
			myerror(ERRFILE, "invalid binary value: %s", value);
			ret = SQLITE_ERROR;
			break;
		    }
		    value[length - 1] = '\0';
		    value++;
		    unsigned int len = (unsigned int)(length / 2 - 1);
		    unsigned char *bin = static_cast<unsigned char *>(xmalloc(len));
		    if (hex2bin(bin, value, len) < 0) {
			free(bin);
			myerror(ERRFILE, "invalid binary value: %s", value);
			ret = SQLITE_ERROR;
			break;
		    }
		    ret = sqlite3_bind_blob(stmt, i + 1, bin, len, free);
		}
		}
	    }

	    if (ret != SQLITE_OK) {
		myerror(ERRDB, "can't bind column");
		array_free(column_types, NULL);
		return -1;
	    }
	}

	if (sqlite3_step(stmt) != SQLITE_DONE) {
	    myerror(ERRDB, "can't insert row into table %s", table_name);
	    array_free(column_types, NULL);
	    return -1;
	}
	if (sqlite3_reset(stmt) != SQLITE_OK) {
	    myerror(ERRDB, "can't reset statement");
	    array_free(column_types, NULL);
	    return -1;
	}
    }

    array_free(column_types, NULL);
    return 0;
}

static void
unget_line(char *line) {
    ungot_line = line;
}
