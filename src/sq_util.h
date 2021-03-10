#ifndef HAD_SQ_UTIL_H
#define HAD_SQ_UTIL_H

/*
  sq_util.h -- sqlite3 utility functions
  Copyright (C) 2007-2020 Dieter Baron and Thomas Klausner

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

#include <vector>

#include <sqlite3.h>

#include "Hashes.h"

std::vector<uint8_t> sq3_get_blob(sqlite3_stmt *stmt, int index);
int sq3_get_int_default(sqlite3_stmt *stmt, int index, int default_value);
void sq3_get_hashes(Hashes *hashes, sqlite3_stmt *stmt, int index);
int64_t sq3_get_int64_default(sqlite3_stmt *stmt, int index, int64_t default_value);
std::string sq3_get_string(sqlite3_stmt *stmt, int index);
uint64_t sq3_get_uint64(sqlite3_stmt *stmt, int col);
uint64_t sq3_get_uint64_default(sqlite3_stmt *stmt, int col, uint64_t def);
int sq3_set_blob(sqlite3_stmt *stmt, int index, const std::vector<uint8_t> &data);
int sq3_set_hashes(sqlite3_stmt *stmt, int index, const Hashes *hashes, int nullp);
int sq3_set_int_default(sqlite3_stmt *stmt, int index, int value, int default_value);
int sq3_set_int64_default(sqlite3_stmt *stmt, int index, int64_t value, int64_t default_value);
int sq3_set_uint64_default(sqlite3_stmt *stmt, int index, uint64_t value, uint64_t default_value);
int sq3_set_uint64(sqlite3_stmt *stmt, int index, uint64_t value);
int sq3_set_string(sqlite3_stmt *stmt, int index, const std::string &value);
#endif /* sq_util.h */
