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

#include "config.h"
#include "compat.h"

#include <cinttypes>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include "DB.h"
#include "error.h"
#include "SharedFile.h"
#include "sq_util.h"
#include "util.h"


static int column_type(const std::string &name);
static int db_type(const std::string &name);
static std::optional<std::string> get_line(FILE *f);
static int restore_db(DB *dbh, FILE *f);
static int restore_table(DB *dbh, FILE *f);
static void unget_line(const std::string &line);
static std::vector<std::string> split(const std::string &string, const std::string &separaptor, bool strip_whitespace = false);


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
struct option options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'V'},
    
    {"type", 1, 0, 't'}
};


int main(int argc, char *argv[]) {
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

    std::string dump_fname = argv[optind];
    std::string db_fname = argv[optind + 1];

    seterrinfo(dump_fname);

    auto f = make_shared_file(dump_fname, "r");
    if (!f) {
	myerror(ERRSTR, "can't open dump '%s'", dump_fname.c_str());
	exit(1);
    }

    try {
        DB dbh(db_fname, type | DBH_TRUNCATE | DBH_WRITE | DBH_CREATE);

        seterrdb(&dbh);

        if (restore_db(&dbh, f.get()) < 0)
            exit(1);
        
        if (type == DBH_FMT_MAME) {
            if (sqlite3_exec(dbh.db, sql_db_init_2, NULL, NULL, NULL) != SQLITE_OK) {
                myerror(ERRDB, "can't create indices");
                exit(1);
            }
        }
    }
    catch (std::exception &e) {
        myerror(ERRDB, "can't create database '%s'", db_fname.c_str());
        exit(1);
    }

    exit(0);
}


static const std::unordered_map<std::string, int> column_types = {
    { "binary", SQLITE_BLOB },
    { "blob", SQLITE_BLOB },
    { "integer", SQLITE_INTEGER },
    { "text", SQLITE_TEXT }
};

static int column_type(const std::string &name) {
    auto it = column_types.find(name);
    
    if (it == column_types.end()) {
        return -1;
    }

    return it->second;
}


static const std::unordered_map<std::string, int> db_types = {
    { "ckmamedb", DBH_FMT_DIR },
    { "mamedb", DBH_FMT_MAME },
    { "memdb", DBH_FMT_MEM }
};

static int db_type(const std::string &name) {
    auto it = db_types.find(name);
    
    if (it == db_types.end()) {
        return -1;
    }

    return it->second;
}


static std::optional<std::string> ungot_line;

static std::optional<std::string> get_line(FILE *f) {
    static char *buffer = NULL;
    static size_t buffer_size = 0;

    if (ungot_line.has_value()) {
	auto line = ungot_line.value();
        ungot_line.reset();
	return line;
    }

    if (getline(&buffer, &buffer_size, f) < 0) {
        return {};
    }

    if (buffer[strlen(buffer) - 1] == '\n')
	buffer[strlen(buffer) - 1] = '\0';
    return buffer;
}

static int
restore_db(DB *dbh, FILE *f) {
    ungot_line.reset();

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
restore_table(DB *dbh, FILE *f) {
    auto next_line = get_line(f);

    if (!next_line.has_value()) {
	return 0;
    }
    
    auto line = next_line.value();

    if (line.compare(0, 10, ">>> table ") != 0 || line[line.length() - 1] != ')') {
	myerror(ERRFILE, "invalid format of table header");
	return -1;
    }

    auto index = line.find_first_of(' ', 10);
    if (index == std::string::npos || line[index + 1] != '(') {
	myerror(ERRFILE, "invalid format of table header");
	return -1;
    }
    auto table_name = line.substr(10, index - 10);

    index += 2;

    std::string query = "pragma table_info(" + table_name + ")";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(dbh->db, query.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
	myerror(ERRDB, "can't query table %s", table_name.c_str());
	return -1;
    }

    auto columns = split(line.substr(index, line.length() - index - 1), ",", true);
    std::vector<int> column_types;
    int ret;
    size_t i = 0;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (i >= columns.size()) {
	    myerror(ERRFILE, "too few columns in dump for table %s", table_name.c_str());
	    return -1;
	}
        if (columns[i] != reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1))) {
            myerror(ERRFILE, "column '%s' in dump doesn't match column '%s' in db for table '%s'", columns[i].c_str(), sqlite3_column_text(stmt, 1), table_name.c_str());
	    return -1;
	}

	int coltype = column_type(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
	if (coltype < 0) {
	    myerror(ERRDB, "unsupported column type %s for column %s of table %s", sqlite3_column_text(stmt, 1), columns[i].c_str(), table_name.c_str());
	    return -1;
	}
	column_types.push_back(coltype);
        i += 1;
    }

    sqlite3_finalize(stmt);

    query = "insert into " + table_name + " values (";
    for (size_t i = 0; i < column_types.size(); i++) {
        if (i != 0) {
            query += ", ";
        }
        query += "?";
    }
    query += ")";

    if (sqlite3_prepare_v2(dbh->db, query.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
	myerror(ERRDB, "can't prepare insert statement for table %s", table_name.c_str());
	return -1;
    }

    while ((next_line = get_line(f)).has_value()) {
        line = next_line.value();
        
        if (line.compare(0, 10, ">>> table ") == 0) {
	    unget_line(line);
	    break;
	}

        auto values = split(line, "|");
        
        if (values.size() < column_types.size()) {
            myerror(ERRFILE, "too few columns in row for table %s", table_name.c_str());
            return -1;
        }
        
        for (size_t i = 0; i < column_types.size(); i++) {
            auto &value = values[i];
            
            if (value == "<null>") {
                ret = sqlite3_bind_null(stmt, static_cast<int>(i + 1));
            }
            else {
		switch (column_types[i]) {
                    case SQLITE_TEXT:
                        ret = sq3_set_string(stmt, static_cast<int>(i + 1), values[i]);
                        break;
                        
                    case SQLITE_INTEGER: {
                        intmax_t vi = strtoimax(value.c_str(), NULL, 10);
                        ret = sqlite3_bind_int64(stmt, static_cast<int>(i + 1), vi);
                        break;
                    }

                    case SQLITE_BLOB: {
                        size_t length = value.length();

                        if (value[0] != '<' || value[length - 1] != '>' || length % 2) {
                            myerror(ERRFILE, "invalid binary value: %s", value.c_str());
                            ret = SQLITE_ERROR;
                            break;
                        }
                        
                        size_t len = length / 2 - 1;
                        unsigned char *bin = static_cast<unsigned char *>(malloc(len));
                        if (bin == NULL) {
                            myerror(ERRSTR, "cannot allocate memory");
                            return -1;
                        }
                        if (hex2bin(bin, value.substr(1, length - 2).c_str(), len) < 0) {
                            free(bin);
                            myerror(ERRFILE, "invalid binary value: %s", value.c_str());
                            ret = SQLITE_ERROR;
                            break;
                        }
                        ret = sqlite3_bind_blob(stmt, static_cast<int>(i + 1), bin, static_cast<int>(len), free);
                        break;
                    }
                }
            }

	    if (ret != SQLITE_OK) {
		myerror(ERRDB, "can't bind column");
		return -1;
	    }
	}

	if (sqlite3_step(stmt) != SQLITE_DONE) {
	    myerror(ERRDB, "can't insert row into table %s", table_name.c_str());
	    return -1;
	}
	if (sqlite3_reset(stmt) != SQLITE_OK) {
	    myerror(ERRDB, "can't reset statement");
	    return -1;
	}
    }

    return 0;
}

static void unget_line(const std::string &line) {
    ungot_line = line;
}


static std::vector<std::string> split(const std::string &string, const std::string &separaptor, bool strip_whitespace) {
    std::vector<std::string> result;

    size_t length = string.length();
    size_t separator_length = separaptor.length();
    size_t start = 0;
    size_t end;
    while ((end = string.find(separaptor, start)) != std::string::npos) {
        result.push_back(string.substr(start, end - start));
        start = end + separator_length;
        if (strip_whitespace) {
            while (start < length && isspace(string[start])) {
                start += 1;
            }
        }
    }
    
    result.push_back(string.substr(start));

    return result;
}
