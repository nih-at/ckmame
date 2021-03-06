/*
  sq_util.c -- sqlite3 utility functions
  Copyright (C) 2010-2013 Dieter Baron and Thomas Klausner

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

#include "sq_util.h"

#include <climits>

#include "Exception.h"

std::vector<uint8_t> sq3_get_blob(sqlite3_stmt *stmt, int col) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
        return {};
    }

    auto size = sqlite3_column_bytes(stmt, col);
    auto bytes = reinterpret_cast<const uint8_t *>(sqlite3_column_blob(stmt, col));
    
    return std::vector<uint8_t>(bytes, bytes + size);
}


void sq3_get_hashes(Hashes *hashes, sqlite3_stmt *stmt, int col) {
    if (sqlite3_column_type(stmt, col) != SQLITE_NULL) {
	hashes->crc = sqlite3_column_int64(stmt, col) & 0xffffffff;
	hashes->types |= Hashes::TYPE_CRC;
    }
    if (sqlite3_column_type(stmt, col + 1) != SQLITE_NULL) {
        hashes->set(Hashes::TYPE_MD5, sqlite3_column_blob(stmt, col + 1));
    }
    if (sqlite3_column_type(stmt, col + 2) != SQLITE_NULL) {
        hashes->set(Hashes::TYPE_SHA1, sqlite3_column_blob(stmt, col + 2));
    }
}


int sq3_get_int_default(sqlite3_stmt *stmt, int col, int def) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL)
	return def;
    return sqlite3_column_int(stmt, col);
}


int64_t sq3_get_int64_default(sqlite3_stmt *stmt, int col, int64_t def) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
	return def;
    }
    return sqlite3_column_int64(stmt, col);
}

uint64_t sq3_get_uint64(sqlite3_stmt *stmt, int col) {
    return static_cast<uint64_t>(sqlite3_column_int64(stmt, col));
}

uint64_t sq3_get_uint64_default(sqlite3_stmt *stmt, int col, uint64_t def) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
        return def;
    }
    return static_cast<uint64_t>(sqlite3_column_int64(stmt, col));
}

std::string sq3_get_string(sqlite3_stmt *stmt, int i) {
    if (sqlite3_column_type(stmt, i) == SQLITE_NULL)
	return "";

    return reinterpret_cast<const char *>(sqlite3_column_text(stmt, i));
}


int sq3_set_blob(sqlite3_stmt *stmt, int col, const std::vector<uint8_t> &value) {
    if (value.empty()) {
        return sqlite3_bind_null(stmt, col);
    }
    if (value.size() > INT_MAX) {
	return SQLITE_TOOBIG;
    }
    return sqlite3_bind_blob(stmt, col, value.data(), static_cast<int>(value.size()), SQLITE_STATIC);
}


int sq3_set_hashes(sqlite3_stmt *stmt, int col, const Hashes *hashes, int nullp) {
    int ret;

    ret = SQLITE_OK;
    
    if (hashes->has_type(Hashes::TYPE_CRC)) {
        ret = sqlite3_bind_int64(stmt, col++, hashes->crc);
    }
    else if (nullp) {
        ret = sqlite3_bind_null(stmt, col++);
    }
    if (ret != SQLITE_OK) {
	return ret;
    }

    if (hashes->has_type(Hashes::TYPE_MD5)) {
        ret = sqlite3_bind_blob(stmt, col++, hashes->md5, Hashes::SIZE_MD5, SQLITE_STATIC);
    }
    else if (nullp) {
        ret = sqlite3_bind_null(stmt, col++);
    }
    if (ret != SQLITE_OK) {
	return ret;
    }

    if (hashes->has_type(Hashes::TYPE_SHA1)) {
        ret = sqlite3_bind_blob(stmt, col++, hashes->sha1, Hashes::SIZE_SHA1, SQLITE_STATIC);
    }
    else if (nullp) {
	ret = sqlite3_bind_null(stmt, col++);
    }

    return ret;
}


int sq3_set_int_default(sqlite3_stmt *stmt, int col, int val, int def) {
    if (val == def)
	return sqlite3_bind_null(stmt, col);
    return sqlite3_bind_int(stmt, col, val);
}


int sq3_set_int64_default(sqlite3_stmt *stmt, int col, int64_t val, int64_t def) {
    if (val == def)
	return sqlite3_bind_null(stmt, col);
    return sqlite3_bind_int64(stmt, col, val);
}


int sq3_set_string(sqlite3_stmt *stmt, int i, const std::string &s) {
    if (!s.empty()) {
	return sqlite3_bind_text(stmt, i, s.c_str(), -1, SQLITE_STATIC);
    }
    else {
	return sqlite3_bind_null(stmt, i);
    }
}


int sq3_set_uint64(sqlite3_stmt *stmt, int col, uint64_t val) {
    return sqlite3_bind_int64(stmt, col, static_cast<int64_t>(val));
}

int sq3_set_uint64_default(sqlite3_stmt *stmt, int col, uint64_t val, uint64_t def) {
    if (val == def) {
        return sqlite3_bind_null(stmt, col);
    }
    return sqlite3_bind_int64(stmt, col, static_cast<int64_t>(val));
}


void sq3_set_string(sqlite3_stmt *stmt, const std::string &name, const std::string &value) {
    auto index = sqlite3_bind_parameter_index(stmt, name.c_str());
    
    if (index == 0) {
        throw Exception("invalid parameter '" + name + "'");
    }
    
    if (sq3_set_string(stmt, index, value) != SQLITE_OK) {
        throw Exception("can't bind parameter '" + name + "'");
    }
}
