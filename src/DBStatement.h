#ifndef HAD_DB_STATEMENT_H
#define HAD_DB_STATEMENT_H

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
#include <vector>

#include <sqlite3.h>

#include "Hashes.h"

class DB;

class DBStatement {
public:
    DBStatement(DB *db, const std::string &sql_query);
    ~DBStatement();
        
    void execute();
    bool step();
    void reset();

    std::vector<uint8_t> get_blob(const std::string &name);
    Hashes get_hashes();
    int get_int(const std::string &name);
    int get_int(const std::string &name, int default_value);
    int64_t get_int64(const std::string &name);
    int64_t get_int64(const std::string &name, int64_t default_value);
    int64_t get_rowid();
    std::string get_string(const std::string &name);
    uint64_t get_uint64(const std::string &name) { return static_cast<uint64_t>(get_int64(name)); }
    uint64_t get_uint64(const std::string &name, uint64_t default_value) { return static_cast<uint64_t>(get_int64(name, static_cast<int64_t>(default_value))); }

    void set_blob(const std::string &name, const std::vector<uint8_t> &data);
    void set_hashes(const Hashes &hashes, bool set_null);
    void set_int(const std::string &name, int value);
    void set_int(const std::string &name, int value, int default_value);
    void set_int64(const std::string &name, int64_t value);
    void set_int64(const std::string &name, int64_t value, int64_t default_value);
    void set_null(const std::string &name);
    void set_string(const std::string &name, const std::string &value, bool store_empty_string = false);
    void set_uint64(const std::string &name, uint64_t value) { set_int64(name, static_cast<int64_t>(value)); }
    void set_uint64(const std::string &name, uint64_t value, uint64_t default_value) { set_int64(name, static_cast<int64_t>(value), static_cast<int64_t>(default_value)); }
    
private:
    int get_column_index(const std::string &name);
    int get_parameter_index(const std::string &name);
    
    DB *db;
    sqlite3_stmt *stmt;
    std::unordered_map<std::string, int> column_names;
    std::unordered_map<std::string, int> parameter_names;
};


#endif // HAD_DB_STATEMENT_H
