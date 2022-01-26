/*
  dbdump.c -- print contents of db
  Copyright (C) 2005-2014 Dieter Baron and Thomas Klausner

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

#include "compat.h"

#include <cstring>
#include <filesystem>
#include <sqlite3.h>

#include "error.h"
#include "Exception.h"
#include "util.h"


const char *usage = "usage: %s db-file\n";

static void dump_db(sqlite3 *);
static void dump_table(sqlite3 *, const std::string &table_name);


int
main(int argc, char *argv[]) {
    char *fname;

    setprogname(argv[0]);

    if (argc != 2) {
	fprintf(stderr, usage, getprogname());
	exit(1);
    }

    fname = argv[1];

    seterrinfo(fname);

    if (!std::filesystem::exists(fname)) {
	myerror(ERRSTR, "database '%s' not found", fname);
	exit(1);
    }

    sqlite3 *db;
    if (sqlite3_open(fname, &db) != SQLITE_OK) {
	/* seterrdb(db); */
	myerror(ERRDB, "can't open database '%s'", fname);
	sqlite3_close(db);
	exit(1);
    }

    /* seterrdb(db); */

    try {
        dump_db(db);
    }
    catch (std::exception &exception) {
        myerror(ERRDB, "can't dump database '%s': %s", fname, exception.what());
        exit(1);
    }

    sqlite3_close(db);

    exit(0);
}


static void dump_db(sqlite3 *db) {
    auto stmt = DBStatement(db, "select name from sqlite_master where type='table' and name not like 'sqlite_%' order by name");

    while (stmt.step()) {
        dump_table(db, stmt.get_string("name"));
    }
}


static void dump_table(sqlite3 *db, const std::string &table_name) {
    int i, ret;

    std::string query = "select * from " + table_name;
    printf(">>> table %s (", table_name.c_str());
    
    {
        auto stmt = DBStatement(db, "pragma table_info(" + table_name + ")");
        
        auto first_col = true;
        auto first_key = true;

        while (stmt.step()) {
            printf("%s%s", first_col ? "" : ", ", stmt.get_string("name").c_str());
            first_col = false;
            
            if (stmt.get_int("pk") != 0) {
                query += (first_key ? " order by " : ", ") + stmt.get_string("name");
                first_key = false;
            }
        }
    }

    printf(")\n");

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw Exception("can't select rows for table '" + table_name + "': " + sqlite3_errmsg(db));
    }

    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
	for (i = 0; i < sqlite3_column_count(stmt); i++) {
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
                case SQLITE_BLOB: {
                    auto ptr = static_cast<const uint8_t *>(sqlite3_column_blob(stmt, i));
                    auto bin = std::vector<uint8_t>(ptr, ptr + static_cast<size_t>(sqlite3_column_bytes(stmt, i)));
                    printf("<%s>", bin2hex(bin).c_str());
                    break;
                }
	    }
	}
	printf("\n");
    }

    sqlite3_finalize(stmt);

    if (ret != SQLITE_DONE) {
        throw Exception("can't select rows for table '" + table_name + "': " + sqlite3_errmsg(db));
    }
}
