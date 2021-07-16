/*
 DBStatement.cc -- object wrapper around sqlite3_stmt.
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

#include "DBStatement.h"


#include "DB.h"
#include "Exception.h"

DBStatement::DBStatement(DB *db, const std::string &sql_query) {
    if (sqlite3_prepare_v2(db->db, sql_query.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        throw Exception("can't create SQL statement '" + sql_query + "'"); // TODO: include sqlite error
    }
    
    auto num_columns = sqlite3_column_count(stmt);
    for (int i = 0; i < num_columns; i++) {
        column_names[sqlite3_column_name(stmt, i)] = i;
    }
    
    auto num_paramters = sqlite3_bind_parameter_count(stmt);
    for (int i = 1; i <= num_paramters; i++) {
        parameter_names[sqlite3_bind_parameter_name(stmt, i)] = i;
    }
}

DBStatement::~DBStatement() {
    sqlite3_finalize(stmt);
}


void DBStatement::set_string(const std::string &name, const std::string &value) {
    auto index = get_parameter_index(name);
    
    int ret;
    
    if (!value.empty()) {
        ret = sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_STATIC);
    }
    else {
        ret = sqlite3_bind_null(stmt, index);
    }
    
    if (ret != SQLITE_OK) {
        throw Exception("can't bind parameter '" + name + "'");
    }
}


void DBStatement::reset() {
    if (sqlite3_reset(stmt) != SQLITE_OK || sqlite3_clear_bindings(stmt) != SQLITE_OK) {
        throw Exception("can't reset SQL statement"); // TODO: include sqlite error
    }
}


bool DBStatement::step() {
    switch (sqlite3_step(stmt)) {
        case SQLITE_DONE:
            return false;

        case SQLITE_ROW:
            return true;
            
        default:
            throw Exception("error executing statement"); // TODO: detail
    }
}


int DBStatement::get_column_index(const std::string &name) {
    auto it = column_names.find(name);
    
    if (it == column_names.end()) {
        throw Exception("unknown column '" + name + "'");
    }
    
    return it->second;
}


int DBStatement::get_parameter_index(const std::string &name) {
    auto it = parameter_names.find(name);
    
    if (it == parameter_names.end()) {
        throw Exception("unknown parameter '" + name + "'");
    }
    
    return it->second;
}
