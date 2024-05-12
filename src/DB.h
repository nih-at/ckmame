#ifndef HAD_DB_H
#define HAD_DB_H

/*
  DB.h -- object layer around SQLite3
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

#include <filesystem>
#include <string>
#include <utility>

#include <sqlite3.h>

#include "DBStatement.h"
#include "Hashes.h"


#define DBH_READ 0x00                                   /* open readonly */
#define DBH_WRITE 0x10                                  /* open for writing */
#define DBH_CREATE 0x20                                 /* create database if it doesn't exist */
#define DBH_TRUNCATE 0x40                               /* delete database if it exists */
#define DBH_NEW (DBH_CREATE | DBH_TRUNCATE | DBH_WRITE) /* create new writable empty database */

enum dbh_list { DBH_KEY_LIST_DISK, DBH_KEY_LIST_GAME };

// This should be a private nested class of DB, but C++ hash support is utterly stupid and doesn't support that.
class MigrationVersions {
public:
    MigrationVersions(int old_version_, int new_version_) : old_version(old_version_), new_version(new_version_) { }
    int old_version;
    int new_version;
    
    bool operator==(const MigrationVersions &other) const {
        return old_version == other.old_version && new_version == other.new_version;
    }
};

// #pragma hide_this_forever begin
namespace std {
template<> struct hash<MigrationVersions> {
    size_t operator()(const MigrationVersions &a) const {
        return hash<int>()(a.old_version) ^ hash<int>()(a.new_version);
    }
};
}
// #pragma hide_this_forever end


// This should be a private nested class of DB, but C++ hash support is utterly stupid and doesn't support that.
class StatementID {
public:
    explicit StatementID(int name_) : name(name_), flags(0) { }
    StatementID(int name_, const Hashes &hashes, bool have_size_) : name(name_), flags(hashes.get_types() | (have_size_ ? have_size : 0) | parameterized) { }
    
    bool operator==(const StatementID &other) const { return name == other.name && flags == other.flags; }
    
    [[nodiscard]] bool is_parameterized() const { return flags & parameterized; }
    [[nodiscard]] bool has_size() const { return flags & have_size; }
    [[nodiscard]] bool hash_types() const { return flags & 0xffff; }
    [[nodiscard]] bool has_hash(int type) const { return flags & type; }
    
    int name;
    int flags;
    
private:
    static const int parameterized;
    static const int have_size;
};

namespace std {
template<> struct hash<StatementID> {
    size_t operator()(const StatementID &a) const {
        return hash<int>()(a.name) ^ hash<int>()(a.flags);
    }
};
}

class DB {
public:
    class DBFormat {
    public:
        int id;
        int version;
        std::string init_sql;
        std::unordered_map<MigrationVersions, std::string> migrations;
    };

    DB(const DBFormat &format, std::string filename, int mode);
    virtual ~DB();
    
    sqlite3 *db;
    
    [[nodiscard]] std::string error() const;
    
    // This is used by dbrestore to create databases with arbitrary schema and version.
    static void upgrade(sqlite3 *db, int format, int version, const std::string &statement);
    
    // This needs to be public to make it hashable.
    
protected:
    static std::filesystem::path make_db_file_name(const std::filesystem::path& directory, const std::string& name, bool use_central_cache);
    DBStatement *get_statement_internal(int name);
    DBStatement *get_statement_internal(int name, const Hashes &hashes, bool have_size);
    
    [[nodiscard]] virtual std::string get_query(int name, bool parameterized) const { return ""; }
    [[nodiscard]] virtual const std::unordered_map<MigrationVersions, std::string> &get_migrations() const { return no_migrations; }
    
private:
    class MigrationStep {
    public:
        MigrationStep(std::string statement_, int version_) : statement(std::move(statement_)), version(version_) { }
        std::string statement;
        int version;
    };
    
    static const std::unordered_map<MigrationVersions, std::string> no_migrations;
    
    DBStatement *get_statement_internal(StatementID statement_id);
    
    [[nodiscard]] int get_version(const DBFormat &format) const;
    void check_version(const DBFormat &format);
    void open(const DBFormat &format, int sql3_flags, bool needs_init);
    void close();
    void migrate(const DBFormat &format, int from_version, int to_version);
    void upgrade(int format, int version, const std::string &statement) const;

    std::string filename;
    std::unordered_map<StatementID, std::shared_ptr<DBStatement>> statements;
};

#endif // HAD_DB_H
