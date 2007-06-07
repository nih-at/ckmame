/*
  $NiH$

  r_detector.c -- read detector from db
  Copyright (C) 2007 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stdlib.h>

#include "dbh.h"
#include "detector.h"
#include "sq_util.h"

#define QUERY_DAT	\
    "select name, author, version from dat where dat_idx = -1"
#define QUERY_RULE	\
    "select rule_idx, start_offset, end_offset, operation" \
    " from rule order by rule_idx"
#define QUERY_TEST	\
    "select type, offset, size, mask, value, result from test" \
    " where rule_idx = ? order by test_idx"

static int r_rules(detector_t *, sqlite3_stmt *, sqlite3_stmt *);



detector_t *
r_detector(sqlite3 *db)
{
    sqlite3_stmt *stmt, *stmt2;
    detector_t *d;
    int ret;

    if (sqlite3_prepare_v2(db, QUERY_DAT, -1, &stmt, NULL) != SQLITE_OK)
	return NULL;

    if (sqlite3_step(stmt) != SQLITE_ROW) {
	sqlite3_finalize(stmt);
	return NULL;
    }

    d = detector_new();

    detector_name(d) = sq3_get_string(stmt, 0);
    detector_author(d) = sq3_get_string(stmt, 1);
    detector_version(d) = sq3_get_string(stmt, 2);

    sqlite3_finalize(stmt);

    if (sqlite3_prepare_v2(db, QUERY_RULE, -1, &stmt, NULL) != SQLITE_OK)
	return NULL;
    if (sqlite3_prepare_v2(db, QUERY_TEST, -1, &stmt2, NULL) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return NULL;
    }

    ret = r_rules(d, stmt, stmt2);

    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt2);

    if (ret < 0) {
	detector_free(d);
	return NULL;
    }

    return d;
}



static int
r_rules(detector_t *d, sqlite3_stmt *st_r, sqlite3_stmt *st_t)
{
    array_t *rs, *ts;
    detector_rule_t *r;
    detector_test_t *t;
    int ret;
    int idx;
    size_t lmask, lvalue;

    rs = detector_rules(d);
    
    while ((ret=sqlite3_step(st_r)) == SQLITE_ROW) {
	r = (detector_rule_t *)array_grow(rs, detector_rule_init);

	idx = sqlite3_column_int(st_r, 0);
	detector_rule_start_offset(r) = sq3_get_int64_default(st_r, 1, 0);
	detector_rule_end_offset(r)
	    = sq3_get_int64_default(st_r, 2, DETECTOR_OFFSET_EOF);
	detector_rule_operation(r)
	    = sq3_get_int_default(st_r, 3, DETECTOR_OP_NONE);

	if (sqlite3_bind_int(st_t, 1, idx) != SQLITE_OK)
	    return -1;

	ts = detector_rule_tests(r);
	
	while ((ret=sqlite3_step(st_t)) == SQLITE_ROW) {
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
	if (ret != SQLITE_DONE
	    || sqlite3_reset(st_t) != SQLITE_OK)
	    return -1;
    }

    return 0;
}
