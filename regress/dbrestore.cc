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

#include <ProgramName.h>
#include <cinttypes>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "CkmameDB.h"
#include "DB.h"
#include "Exception.h"
#include "RomDB.h"
#include "SharedFile.h"
#include "util.h"
#include "globals.h"

enum DBType {
    DBTYPE_INVALID = -1,
    DBTYPE_CKMAMEDB,
    DBTYPE_ROMDB
};

static int column_type(const std::string &name);
static int db_format(DBType type);
static DBType db_type(const std::string &name);
static std::optional<std::string> get_line(FILE *f);
static int restore_db(sqlite3 *db, FILE *f);
static int restore_table(sqlite3 *db, FILE *f);
static void unget_line(const std::string &line);
static std::vector<std::string> split(const std::string &string, const std::string &separaptor, bool strip_whitespace = false);

std::vector<Commandline::Option> dbrestore_options = {
    Commandline::Option("db-version", "version", "specify DB schema version"),
    Commandline::Option("sql","file", "take SQL schema from FILE"),
    Commandline::Option("type", 't', "type", "specify type of database: mamedb (default) or ckmamedb)")
};

#define PROGRAM_NAME "dbrestore"

int main(int argc, char *argv[]) {
    DBType type = DBTYPE_ROMDB;
    std::string sql_file;
    int db_version = -1;

    const char *header = PROGRAM_NAME " by Dieter Baron and Thomas Klausner";
    const char *footer = "Report bugs to " PACKAGE_BUGREPORT ".";
    const char *version = "dumpgame (" PACKAGE " " VERSION ")\nCopyright (C) 2021-2024 Dieter Baron and Thomas Klausner\n"
        PACKAGE " " VERSION "\n"
        PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

    auto commandline = Commandline(dbrestore_options, "dump-file db-file", header, footer, version);

    auto arguments = commandline.parse(argc, argv);

    for (const auto& option: arguments.options) {
        if (option.name == "db-version") {
            try {
                size_t idx;
                db_version = std::stoi(option.argument, &idx);
                if (option.argument[idx] != '\0') {
                    throw std::invalid_argument("");
                }
            }
            catch (...) {
                fprintf(stderr, "%s: invalid DB schema version '%s'\n", ProgramName::get().c_str(), option.argument.c_str());
                exit(1);
            }
        }
        else if (option.name == "sql") {
            sql_file = option.argument;
        }
        else if (option.name == "type") {
            if ((type = db_type(option.argument)) == DBTYPE_INVALID) {
                fprintf(stderr, "%s: unknown db type '%s'\n", ProgramName::get().c_str(), option.argument.c_str());
                exit(1);
            }
        }
    }

    if (arguments.arguments.size() != 2) {
        commandline.usage(false, stderr);
	exit(1);
    }

    std::string dump_fname = arguments.arguments[0];
    std::string db_fname = arguments.arguments[1];

    output.set_error_file(dump_fname);

    auto f = make_shared_file(dump_fname, "r");
    if (!f) {
	output.error_system("can't open dump '%s'", dump_fname.c_str());
	exit(1);
    }

    try {
        if (!sql_file.empty()) {
            std::string sql_statement = slurp(sql_file);
            sqlite3 *db;
            
            if (sqlite3_open_v2(db_fname.c_str(), &db, SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
                throw std::invalid_argument("");
            }

            try {
                DB::upgrade(db, db_format(type), db_version, sql_statement);
            }
            catch (Exception &e) {
                sqlite3_close(db);
                std::filesystem::remove(db_fname);
                throw;
            }
            
            if (restore_db(db, f.get()) < 0) {
                exit(1);
            }
        }
        else {
            std::unique_ptr<DB> db;
            
            switch (type) {
                case DBTYPE_CKMAMEDB:
                    db = std::make_unique<CkmameDB>(db_fname, ".", FILE_NOWHERE);
                    break;

                case DBTYPE_ROMDB:
                    db = std::make_unique<RomDB>(db_fname, DBH_TRUNCATE | DBH_WRITE | DBH_CREATE);
                    break;
                    
                default:
                    throw Exception("can't happen");
            }
            
            output.set_error_database(db.get());
            
            if (restore_db(db->db, f.get()) < 0) {
                exit(1);
            }
            
            if (type == DBTYPE_ROMDB) {
                static_cast<RomDB *>(db.get())->init2();
            }
            
            output.set_error_database(nullptr);
        }
    }
    catch (std::exception &e) {
        output.error_database("can't create database '%s': %s", db_fname.c_str(), e.what());
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


static const std::unordered_map<std::string, DBType> db_types = {
    { "ckmamedb", DBTYPE_CKMAMEDB },
    { "mamedb", DBTYPE_ROMDB },
};

static DBType db_type(const std::string &name) {
    auto it = db_types.find(name);
    
    if (it == db_types.end()) {
        return DBTYPE_INVALID;
    }

    return it->second;
}

static int db_format(DBType type) {
    switch (type) {
        case DBTYPE_CKMAMEDB:
            return CkmameDB::format.id;

        case DBTYPE_ROMDB:
            return RomDB::format.id;
            
        default:
            return -1;
    }
}


static std::optional<std::string> ungot_line;

static std::optional<std::string> get_line(FILE *f) {
    static char *buffer = nullptr;
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
	    output.file_error_system("read error");
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
	output.file_error("invalid format of table header");
	return -1;
    }

    auto index = line.find_first_of(' ', 10);
    if (index == std::string::npos || line[index + 1] != '(') {
	output.file_error("invalid format of table header");
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
                output.file_error("too few columns in dump for table %s", table_name.c_str());
                return -1;
            }
            if (columns[i] != stmt.get_string("name")) {
                output.file_error("column '%s' in dump doesn't match column '%s' in db for table '%s'", columns[i].c_str(), stmt.get_string("name").c_str(), table_name.c_str());
                return -1;
            }
            
            int coltype = column_type(string_lower(stmt.get_string("type")));
            if (coltype < 0) {
                output.error_database("unsupported column type %s for column %s of table %s", stmt.get_string("type").c_str(), columns[i].c_str(), table_name.c_str());
                return -1;
            }
            column_types.push_back(coltype);
            i += 1;
        }
        if (i != columns.size()) {
            output.file_error("too many columns in dump for table %s", table_name.c_str());
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
            output.file_error("too few columns in row for table %s", table_name.c_str());
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
                        intmax_t vi = strtoimax(value.c_str(), nullptr, 10);
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
