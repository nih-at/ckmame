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

#include <stddef.h>

#include "sq_util.h"
#include "xmalloc.h"


void *
sq3_get_blob(sqlite3_stmt *stmt, int col, size_t *sizep)
{
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
	*sizep = 0;
	return NULL;
    }

    *sizep = sqlite3_column_bytes(stmt, col);
    return xmemdup(sqlite3_column_blob(stmt, col), *sizep);
}


void
sq3_get_hashes(hashes_t *h, sqlite3_stmt *stmt, int col)
{
    if (sqlite3_column_type(stmt, col) != SQLITE_NULL) {
	hashes_crc(h) = sqlite3_column_int64(stmt, col) & 0xffffffff;
	hashes_types(h) |= HASHES_TYPE_CRC;
    }
    if (sqlite3_column_type(stmt, col+1) != SQLITE_NULL) {
	memcpy(h->md5, sqlite3_column_blob(stmt, col+1),
	       HASHES_SIZE_MD5);
	hashes_types(h) |= HASHES_TYPE_MD5;
    }
    if (sqlite3_column_type(stmt, col+2) != SQLITE_NULL) {
	memcpy(h->sha1, sqlite3_column_blob(stmt, col+2),
	       HASHES_SIZE_SHA1);
	hashes_types(h) |= HASHES_TYPE_SHA1;
    }
}


int
sq3_get_int_default(sqlite3_stmt *stmt, int col, int def)
{
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL)
	return def;
    return sqlite3_column_int(stmt, col);
}


int64_t
sq3_get_int64_default(sqlite3_stmt *stmt, int col, int64_t def)
{
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL)
	return def;
    return sqlite3_column_int64(stmt, col);
}


char *
sq3_get_string(sqlite3_stmt *stmt, int i)
{
    if (sqlite3_column_type(stmt, i) == SQLITE_NULL)
	return NULL;
    
    return xstrdup((const char *)sqlite3_column_text(stmt, i));
}


int
sq3_set_blob(sqlite3_stmt *stmt, int col, const void *p, size_t s)
{
    if (s == 0 || p == NULL)
	return sqlite3_bind_null(stmt, col);
    return sqlite3_bind_blob64(stmt, col, p, s, SQLITE_STATIC);
}


int
sq3_set_hashes(sqlite3_stmt *stmt, int col, const hashes_t *h, int nullp)
{
    int ret;

    ret = SQLITE_OK;

    if (hashes_has_type(h, HASHES_TYPE_CRC))
	ret = sqlite3_bind_int64(stmt, col++, hashes_crc(h));
    else if (nullp)
	ret = sqlite3_bind_null(stmt, col++);
    if (ret != SQLITE_OK)
	return ret;

    if (hashes_has_type(h, HASHES_TYPE_MD5))
	ret = sqlite3_bind_blob(stmt, col++, h->md5, HASHES_SIZE_MD5,
				SQLITE_STATIC);
    else if (nullp)
	ret = sqlite3_bind_null(stmt, col++);
    if (ret != SQLITE_OK)
	return ret;

    if (hashes_has_type(h, HASHES_TYPE_SHA1))
	ret = sqlite3_bind_blob(stmt, col++, h->sha1, HASHES_SIZE_SHA1,
				SQLITE_STATIC);
    else if (nullp)
	ret = sqlite3_bind_null(stmt, col++);

    return ret;
}


int
sq3_set_int_default(sqlite3_stmt *stmt, int col, int val, int def)
{
    if (val == def)
	return sqlite3_bind_null(stmt, col);
    return sqlite3_bind_int(stmt, col, val);
}


int
sq3_set_int64_default(sqlite3_stmt *stmt, int col, int64_t val, int64_t def)
{
    if (val == def)
	return sqlite3_bind_null(stmt, col);
    return sqlite3_bind_int64(stmt, col, val);
}


int
sq3_set_string(sqlite3_stmt *stmt, int i, const char *s)
{
    if (s)
	return sqlite3_bind_text(stmt, i, s, -1, SQLITE_STATIC);
    else
	return sqlite3_bind_null(stmt, i);
}
