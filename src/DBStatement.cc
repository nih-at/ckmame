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

#include <climits>

#include "Exception.h"

DBStatement::DBStatement(sqlite3 *db_, const std::string &sql_query) : db(db_) {
    if (sqlite3_prepare_v2(db, sql_query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw Exception("can't create SQL statement '" + sql_query + "'"); // TODO: include sqlite error
    }

    auto num_columns = sqlite3_column_count(stmt);
    for (int i = 0; i < num_columns; i++) {
        column_names[sqlite3_column_name(stmt, i)] = i;
    }
    
    auto num_paramters = sqlite3_bind_parameter_count(stmt);
    for (int i = 1; i <= num_paramters; i++) {
        parameter_names[sqlite3_bind_parameter_name(stmt, i) + 1] = i; // skip leading :
    }
}


DBStatement::~DBStatement() {
    sqlite3_finalize(stmt);
}


void DBStatement::execute() {
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw Exception(std::string("error executing statement: ") + sqlite3_errmsg(db));
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
            throw Exception(std::string("error executing statement: ") + sqlite3_errmsg(db));
    }
}


// MARK: - Getting Values


std::vector<uint8_t> DBStatement::get_blob(const std::string &name) {
    auto index = get_column_index(name);

    if (sqlite3_column_type(stmt, index) == SQLITE_NULL) {
        return {};
    }
    
    auto size = sqlite3_column_bytes(stmt, index);
    auto bytes = reinterpret_cast<const uint8_t *>(sqlite3_column_blob(stmt, index));
    
    return std::vector<uint8_t>(bytes, bytes + size);
}


Hashes DBStatement::get_hashes() {
    Hashes hashes;
    
    for (int type = 1; type <= Hashes::TYPE_MAX; type <<= 1) {
        auto index = get_column_index(Hashes::type_name(type));
        
        if (sqlite3_column_type(stmt, index) == SQLITE_NULL) {
            continue;
        }
        
        switch (type) {
            case Hashes::TYPE_CRC:
                hashes.set_crc(sqlite3_column_int64(stmt, index) & 0xffffffff);
                break;
                
            case Hashes::TYPE_MD5:
                hashes.set_md5(static_cast<const uint8_t *>(sqlite3_column_blob(stmt, index)));
                break;

            case Hashes::TYPE_SHA1:
                hashes.set_sha1(static_cast<const uint8_t *>(sqlite3_column_blob(stmt, index)));
                break;
        }
    }
                       
    return hashes;
}


int DBStatement::get_int(const std::string &name) {
    auto index = get_column_index(name);
    
    return sqlite3_column_int(stmt, index);
}


int DBStatement::get_int(const std::string &name, int default_value) {
    auto index = get_column_index(name);

    if (sqlite3_column_type(stmt, index) == SQLITE_NULL) {
        return default_value;
    }
    
    return sqlite3_column_int(stmt, index);
}


int64_t DBStatement::get_int64(const std::string &name) {
    auto index = get_column_index(name);
    
    return sqlite3_column_int64(stmt, index);
}


int64_t DBStatement::get_int64(const std::string &name, int64_t default_value) {
    auto index = get_column_index(name);
    
    if (sqlite3_column_type(stmt, index) == SQLITE_NULL) {
        return default_value;
    }
    return sqlite3_column_int64(stmt, index);
}


int64_t DBStatement::get_rowid() {
    return sqlite3_last_insert_rowid(db);
}

std::string DBStatement::get_string(const std::string &name) {
    auto index = get_column_index(name);
    
    if (sqlite3_column_type(stmt, index) == SQLITE_NULL)
        return "";
    
    return reinterpret_cast<const char *>(sqlite3_column_text(stmt, index));
}


// MARK: - Setting Values


void DBStatement::set_blob(const std::string &name, const std::vector<uint8_t> &value) {
    auto index = get_parameter_index(name);

    int ret;
    
    if (value.empty()) {
        ret = sqlite3_bind_null(stmt, index);
    }
    else if (value.size() > INT_MAX) {
        ret = SQLITE_TOOBIG;
    }
    else {
        ret = sqlite3_bind_blob(stmt, index, value.data(), static_cast<int>(value.size()), SQLITE_STATIC);
    }
    
    if (ret != SQLITE_OK) {
        throw Exception("can't bind parameter '" + name + "'");
    }
}


void DBStatement::set_hashes(const Hashes &hashes, bool set_null) {
    for (int type = 1; type <= Hashes::TYPE_MAX; type <<= 1) {

        int ret = SQLITE_OK;
        
        if (hashes.has_type(type)) {
            auto index = get_parameter_index(Hashes::type_name(type));
            switch (type) {
                case Hashes::TYPE_CRC:
                    ret = sqlite3_bind_int64(stmt, index, hashes.crc);
                    break;
                    
                case Hashes::TYPE_MD5:
                    ret = sqlite3_bind_blob(stmt, index, hashes.md5.data(), static_cast<int>(Hashes::SIZE_MD5), SQLITE_STATIC);
                    break;
                    
                case Hashes::TYPE_SHA1:
                    ret = sqlite3_bind_blob(stmt, index, hashes.sha1.data(), static_cast<int>(Hashes::SIZE_SHA1), SQLITE_STATIC);
                    break;
            }
        }
        else if (set_null) {
            auto index = get_parameter_index(Hashes::type_name(type));
            ret = sqlite3_bind_null(stmt, index);
        }
        
        if (ret != SQLITE_OK) {
            throw Exception("can't bind parameter '" + Hashes::type_name(type) + "'");
        }
    }
}


void DBStatement::set_int(const std::string &name, int value) {
    auto index = get_parameter_index(name);

    if (sqlite3_bind_int(stmt, index, value) != SQLITE_OK) {
        throw Exception("can't bind parameter '" + name + "'");
    }
}


void DBStatement::set_int(const std::string &name, int value, int default_value) {
    auto index = get_parameter_index(name);

    int ret;
    
    if (value == default_value) {
        ret = sqlite3_bind_null(stmt, index);
    }
    else {
        ret = sqlite3_bind_int(stmt, index, value);
    }
    
    if (ret != SQLITE_OK) {
        throw Exception("can't bind parameter '" + name + "'");
    }
}


void DBStatement::set_int64(const std::string &name, int64_t value) {
    auto index = get_parameter_index(name);

    if (sqlite3_bind_int64(stmt, index, value) != SQLITE_OK) {
        throw Exception("can't bind parameter '" + name + "'");
    }
}


void DBStatement::set_int64(const std::string &name, int64_t value, int64_t default_value) {
    auto index = get_parameter_index(name);

    int ret;
    
    if (value == default_value) {
        ret = sqlite3_bind_null(stmt, index);
    }
    else {
        ret = sqlite3_bind_int64(stmt, index, value);
    }
    
    if (ret != SQLITE_OK) {
        throw Exception("can't bind parameter '" + name + "'");
    }
}


void DBStatement::set_null(const std::string &name) {
    auto index = get_parameter_index(name);

    if (sqlite3_bind_null(stmt, index) != SQLITE_OK) {
        throw Exception("can't bind parameter '" + name + "'");
    }
}


void DBStatement::set_string(const std::string &name, const std::string &value, bool store_empty_string) {
    auto index = get_parameter_index(name);
    
    int ret;
    
    if (!value.empty() || store_empty_string) {
        ret = sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_STATIC);
    }
    else {
        ret = sqlite3_bind_null(stmt, index);
    }
    
    if (ret != SQLITE_OK) {
        throw Exception("can't bind parameter '" + name + "'");
    }
}


// MARK: - Helper Functions

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
