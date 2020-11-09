#ifndef HAD_SQ_UTIL_H
#define HAD_SQ_UTIL_H

/*
  sq_util.h -- sqlite3 utility functions
  Copyright (C) 2007-2014 Dieter Baron and Thomas Klausner

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

#include <cinttypes>

#include <sqlite3.h>

#include "hashes.h"

void *sq3_get_blob(sqlite3_stmt *, int, size_t *);
int sq3_get_int_default(sqlite3_stmt *, int, int);
void sq3_get_hashes(hashes_t *, sqlite3_stmt *, int);
int64_t sq3_get_int64_default(sqlite3_stmt *, int, int64_t);
char *sq3_get_string(sqlite3_stmt *, int);
int sq3_set_blob(sqlite3_stmt *, int, const void *, size_t);
int sq3_set_hashes(sqlite3_stmt *, int, const hashes_t *, int);
int sq3_set_int_default(sqlite3_stmt *, int, int, int);
int sq3_set_int64_default(sqlite3_stmt *, int, int64_t, int64_t);
int sq3_set_string(sqlite3_stmt *, int, const char *);

#endif /* sq_util.h */
