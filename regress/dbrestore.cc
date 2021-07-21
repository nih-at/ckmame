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
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "DB.h"
#include "error.h"
#include "Exception.h"
#include "SharedFile.h"
#include "util.h"


static int column_type(const std::string &name);
static int db_type(const std::string &name);
static std::optional<std::string> get_line(FILE *f);
static int restore_db(sqlite3 *db, FILE *f);
static int restore_table(sqlite3 *db, FILE *f);
std::string slurp(const std::string filename);
static void unget_line(const std::string &line);
static std::vector<std::string> split(const std::string &string, const std::string &separaptor, bool strip_whitespace = false);


#define QUERY_COLS_FMT "pragma table_info(%s)"

const char *usage = "usage: %s [-hV] [--db-version VERSION] [--sql SQL_INIT_FILE] [-t db-type] dump-file db-file\n";

char help_head[] = PACKAGE " by Dieter Baron and Thomas Klausner\n\n";

char help[] = "\n"
              "  --db-version VERSION    specify version of database schema\n"
	      "  -h, --help              display this help message\n"
              "  --sql SQL_INIT_FILE     use table definitions from this SQL init file\n"
	      "  -t, --type TYPE         restore database of type TYPE (ckmamedb, mamedb, memdb)\n"
	      "  -V, --version           display version number\n"
	      "\nReport bugs to " PACKAGE_BUGREPORT ".\n";

char version_string[] = PACKAGE " " VERSION "\n"
				"Copyright (C) 2014-2021 Dieter Baron and Thomas Klausner\n" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";


#define OPTIONS "ht:V"

enum {
    OPT_DB_VERSION = 256,
    OPT_SQL
};

struct option options[] = {
    {"db-version", 1, 0, OPT_DB_VERSION },
    {"help", 0, 0, 'h'},
    {"sql", 1, 0, OPT_SQL },
    {"type", 1, 0, 't'},
    {"version", 0, 0, 'V'}
};


int main(int argc, char *argv[]) {
    setprogname(argv[0]);

    int type = DBH_FMT_MAME;
    std::string sql_file;
    int db_version = -1;

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
		fprintf(stderr, "%s: unknown db type '%s'\n", getprogname(), optarg);
		exit(1);
	    }
	    break;
            
        case OPT_DB_VERSION:
            try {
                size_t idx;
                db_version = std::stoi(optarg, &idx);
                if (optarg[idx] != '\0') {
                    throw std::invalid_argument("");
                }
            }
            catch (...) {
                fprintf(stderr, "%s: invalid DB schema version '%s'\n", getprogname(), optarg);
                exit(1);
            }
            break;
            
        case OPT_SQL:
            sql_file = optarg;
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
        if (!sql_file.empty()) {
            std::string sql_statement = slurp(sql_file);
            sqlite3 *db;
            
            if (sqlite3_open_v2(db_fname.c_str(), &db, SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) {
                throw std::invalid_argument("");
            }

            try {
                DB::upgrade(db, type, db_version, sql_statement);
            }
            catch (Exception &e) {
                sqlite3_close(db);
                std::filesystem::remove(db_fname);
                throw e;
            }
            
            if (restore_db(db, f.get()) < 0) {
                exit(1);
            }
        }
        else {
            DB dbh(db_fname, type | DBH_TRUNCATE | DBH_WRITE | DBH_CREATE);
            
            seterrdb(&dbh);
            
            if (restore_db(dbh.db, f.get()) < 0) {
                exit(1);
            }
            
            if (type == DBH_FMT_MAME) {
                if (sqlite3_exec(dbh.db, sql_db_init_2, NULL, NULL, NULL) != SQLITE_OK) {
                    myerror(ERRDB, "can't create indices");
                    exit(1);
                }
            }
        }
    }
    catch (std::exception &e) {
        myerror(ERRDB, "can't create database '%s': %s", db_fname.c_str(), e.what());
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
restore_db(sqlite3 *db, FILE *f) {
    ungot_line.reset();

    while (!feof(f)) {
	if (restore_table(db, f) < 0)
	    return -1;

	if (ferror(f)) {
	    myerror(ERRFILESTR, "read error");
	    return -1;
	}
    }

    return 0;
}


static int
restore_table(sqlite3 *db, FILE *f) {
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


    auto columns = split(line.substr(index, line.length() - index - 1), ",", true);
    std::vector<int> column_types;

    {
        auto stmt = DBStatement(db, "pragma table_info(" + table_name + ")");

        size_t i = 0;
        while (stmt.step()) {
            if (i >= columns.size()) {
                myerror(ERRFILE, "too few columns in dump for table %s", table_name.c_str());
                return -1;
            }
            if (columns[i] != stmt.get_string("name")) {
                myerror(ERRFILE, "column '%s' in dump doesn't match column '%s' in db for table '%s'", columns[i].c_str(), stmt.get_string("name").c_str(), table_name.c_str());
                return -1;
            }
            
            int coltype = column_type(stmt.get_string("type"));
            if (coltype < 0) {
                myerror(ERRDB, "unsupported column type %s for column %s of table %s", stmt.get_string("type").c_str(), columns[i].c_str(), table_name.c_str());
                return -1;
            }
            column_types.push_back(coltype);
            i += 1;
        }
        if (i != columns.size()) {
            myerror(ERRFILE, "too many columns in dump for table %s", table_name.c_str());
            return -1;
        }
    }

    std::string query = "insert into " + table_name + " values (";
    for (size_t i = 0; i < column_types.size(); i++) {
        if (i != 0) {
            query += ", ";
        }
        query += ":" + columns[i];
    }
    query += ")";

    auto stmt = DBStatement(db, query);

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
        
        std::vector<std::vector<uint8_t>> blobs;
        
        for (size_t i = 0; i < column_types.size(); i++) {
            auto &value = values[i];
            
            if (value == "<null>") {
                stmt.set_null(columns[i]);
            }
            else {
		switch (column_types[i]) {
                    case SQLITE_TEXT:
                        stmt.set_string(columns[i], values[i], true);
                        break;
                        
                    case SQLITE_INTEGER: {
                        intmax_t vi = strtoimax(value.c_str(), NULL, 10);
                        stmt.set_int64(columns[i], vi);
                        break;
                    }

                    case SQLITE_BLOB: {
                        size_t length = value.length();

                        if (value[0] != '<' || value[length - 1] != '>' || length % 2) {
                            throw Exception("invalid binary value: %s", value.c_str());
                        }
                        
                        blobs.push_back(hex2bin(value.substr(1, length - 2)));
                        stmt.set_blob(columns[i], blobs[blobs.size() - 1]);
                        break;
                    }
                }
            }
	}

        stmt.execute();
        stmt.reset();
        blobs.clear();
    }

    return 0;
}

static void unget_line(const std::string &line) {
    ungot_line = line;
}


static std::vector<std::string> split(const std::string &string, const std::string &separator, bool strip_whitespace) {
    std::vector<std::string> result;

    size_t length = string.length();
    size_t separator_length = separator.length();
    size_t start = 0;
    size_t end;
    while ((end = string.find(separator, start)) != std::string::npos) {
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

std::string slurp(const std::string filename) {
    auto f = std::ifstream(filename, std::ios::in | std::ios::binary);
        
    const auto size = std::filesystem::file_size(filename);
        
    std::string text(size, '\0');
        
    f.read(text.data(), static_cast<std::streamsize>(size));
        
    return text;
}
