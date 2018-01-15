/*
  romdb_read_detector.c -- read detector from db
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


#include <stdlib.h>

#include "detector.h"
#include "romdb.h"
#include "sq_util.h"

#define QUERY_DAT "select name, author, version from dat where dat_idx = -1"
#define QUERY_RULE                                         \
    "select rule_idx, start_offset, end_offset, operation" \
    " from rule order by rule_idx"
#define QUERY_TEST                                             \
    "select type, offset, size, mask, value, result from test" \
    " where rule_idx = ? order by test_idx"

static int romdb_read_rules(detector_t *, sqlite3_stmt *, sqlite3_stmt *);


detector_t *
romdb_read_detector(romdb_t *db) {
    sqlite3_stmt *stmt, *stmt2;
    detector_t *d;
    int ret;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_DAT_DETECTOR)) == NULL)
	return NULL;

    if (sqlite3_step(stmt) != SQLITE_ROW)
	return NULL;

    d = detector_new();

    detector_name(d) = sq3_get_string(stmt, 0);
    detector_author(d) = sq3_get_string(stmt, 1);
    detector_version(d) = sq3_get_string(stmt, 2);

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_RULE)) == NULL || (stmt2 = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_TEST)) == NULL)
	return NULL;

    ret = romdb_read_rules(d, stmt, stmt2);

    if (ret < 0) {
	detector_free(d);
	return NULL;
    }

    return d;
}


static int
romdb_read_rules(detector_t *d, sqlite3_stmt *st_r, sqlite3_stmt *st_t) {
    array_t *rs, *ts;
    detector_rule_t *r;
    detector_test_t *t;
    int ret;
    int idx;
    size_t lmask, lvalue;

    rs = detector_rules(d);

    while ((ret = sqlite3_step(st_r)) == SQLITE_ROW) {
	r = (detector_rule_t *)array_grow(rs, detector_rule_init);

	idx = sqlite3_column_int(st_r, 0);
	detector_rule_start_offset(r) = sq3_get_int64_default(st_r, 1, 0);
	detector_rule_end_offset(r) = sq3_get_int64_default(st_r, 2, DETECTOR_OFFSET_EOF);
	detector_rule_operation(r) = sq3_get_int_default(st_r, 3, DETECTOR_OP_NONE);

	if (sqlite3_bind_int(st_t, 1, idx) != SQLITE_OK)
	    return -1;

	ts = detector_rule_tests(r);

	while ((ret = sqlite3_step(st_t)) == SQLITE_ROW) {
	    t = (detector_test_t *)array_grow(ts, detector_test_init);

	    detector_test_type(t) = sqlite3_column_int(st_t, 0);
	    detector_test_offset(t) = sqlite3_column_int64(st_t, 1);
	    detector_test_result(t) = sqlite3_column_int64(st_t, 5);

	    switch (detector_test_type(t)) {
	    case DETECTOR_TEST_DATA:
	    case DETECTOR_TEST_OR:
	    case DETECTOR_TEST_AND:
	    case DETECTOR_TEST_XOR:
		detector_test_mask(t) = sq3_get_blob(st_t, 3, &lmask);
		detector_test_value(t) = sq3_get_blob(st_t, 4, &lvalue);
		if (lmask > 0 && lmask != lvalue)
		    return -1;
		detector_test_length(t) = lmask;
		break;
	    case DETECTOR_TEST_FILE_EQ:
	    case DETECTOR_TEST_FILE_LE:
	    case DETECTOR_TEST_FILE_GR:
		detector_test_length(t) = sqlite3_column_int64(st_t, 2);
		break;
	    }
	}
	if (ret != SQLITE_DONE || sqlite3_reset(st_t) != SQLITE_OK)
	    return -1;
    }

    return 0;
}
