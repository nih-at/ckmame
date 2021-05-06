#ifndef _HAD_DBH_H
#define _HAD_DBH_H

/*
  dbh.h -- mame.db sqlite3 data base
  Copyright (C) 1999-2021 Dieter Baron and Thomas Klausner

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

#include <string>

#include <sqlite3.h>

#include "dbh_statements.h"
#include "Hashes.h"


#define DBH_FMT_MAME 0x0 /* mame.db format */
#define DBH_FMT_MEM 0x1  /* in-memory db format */
#define DBH_FMT_DIR 0x2  /* unpacked dirs db format */
#define DBH_FMT(m) ((m)&0xf)

#define DBH_READ 0x00                                   /* open readonly */
#define DBH_WRITE 0x10                                  /* open for writing */
#define DBH_CREATE 0x20                                 /* create database if it doesn't exist */
#define DBH_TRUNCATE 0x40                               /* delete database if it exists */
#define DBH_NEW (DBH_CREATE | DBH_TRUNCATE | DBH_WRITE) /* create new writable empty database */
#define DBH_FLAGS(m) ((m)&0xf0)

enum dbh_list { DBH_KEY_LIST_DISK, DBH_KEY_LIST_GAME, DBH_KEY_LIST_MAX };

#define DBH_DEFAULT_DB_NAME "mame.db"
#define DBH_DEFAULT_OLD_DB_NAME "old.db"
#define DBH_CACHE_DB_NAME ".ckmame.db"

extern const char *sql_db_init[];
extern const char *sql_db_init_2;

class DB {
public:
    DB(const std::string &name, int mode);
    ~DB();
    
    sqlite3 *db;
    sqlite3_stmt *statements[DBH_STMT_MAX];
    int format;
    
    std::string error();

    sqlite3_stmt *get_statement(dbh_stmt_t stmt_id);
    sqlite3_stmt *get_statement(dbh_stmt_t stmt_id, const Hashes *hashes, bool have_size);
    
    // This is used by dbrestore to create databases with arbitrary schema and version.
    static void upgrade(sqlite3 *db, int format, int version, const std::string &statement);
    
    // This needs to be public to make it hashable.
    class MigrationXXX {
    public:
        MigrationXXX(int format_, int old_version_, int new_version_) : format(format_), old_version(old_version_), new_version(new_version_) { }
        int format;
        int old_version;
        int new_version;
        
        bool operator==(const MigrationXXX &other) const {
            return format == other.format && old_version == other.old_version && new_version == other.new_version;
        }
    };

private:
    class MigrationStep {
    public:
        MigrationStep(const std::string &statement_, int version_) : statement(statement_), version(version_) { }
        std::string statement;
        int version;
    };
    
    int get_version();
    void check_version();
    void open(const std::string &name, int sql3_flags, bool needs_init);
    void close();
    void init();
    void migrate(int from_version, int to_version);
    void upgrade(const std::string &statement, int version);

    static std::unordered_map<MigrationXXX, std::string> migrations;
};

// #pragma hide_this_forever begin
namespace std {
    template<> struct hash<DB::MigrationXXX> {
        size_t operator()(const DB::MigrationXXX &a) const {
            return hash<int>()(a.format) ^ hash<int>()(a.old_version) ^ hash<int>()(a.new_version);
        }
    };
}
// #pragma hide_this_forever end

#endif /* dbh.h */
