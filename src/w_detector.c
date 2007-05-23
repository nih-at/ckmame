/*
  $NiH$

  w_detector.c -- write detector to db
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

#define INSERT_DAT	\
    "insert into dat (dat_idx, name, author, version)" \
    " values (-1, ?, ?, ?)"
#define INSERT_RULE	\
    "insert into rule (rule_idx, start_offset, end_offset, operation)" \
    " values (?, ?, ?, ?)"
#define INSERT_TEST	\
    "insert into test (rule_idx, test_idx, type, offset, size, mask," \
    " value, result) values (?, ?, ?, ?, ?, ?, ?, ?)"

static int w_rules(const detector_t *, sqlite3_stmt *, sqlite3_stmt *);



int
w_detector(sqlite3 *db, const detector_t *d)
{
    sqlite3_stmt *stmt, *stmt2;
    int ret;

    if (sqlite3_prepare_v2(db, INSERT_DAT, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    if (sq3_set_string(stmt, 1, detector_name(d)) < 0
	|| sq3_set_string(stmt, 2, detector_author(d)) < 0
	|| sq3_set_string(stmt, 3, detector_version(d)) < 0
	|| sqlite3_step(stmt) != SQLITE_DONE) {
	sqlite3_finalize(stmt);
	return -1;
    }

    sqlite3_finalize(stmt);

    if (sqlite3_prepare_v2(db, INSERT_RULE, -1, &stmt, NULL) != SQLITE_OK)
	return -1;
    if (sqlite3_prepare_v2(db, INSERT_TEST, -1, &stmt2, NULL) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return -1;
    }

    ret = w_rules(d, stmt, stmt2);

    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt2);

    return ret;
}



static int
w_rules(const detector_t *d, sqlite3_stmt *st_r, sqlite3_stmt *st_t)
{
    int i, j;
    detector_rule_t *r;
    detector_test_t *t;

    for (i=0; i<detector_num_rules(d); i++) {
	r = detector_rule(d, i);

	if (sqlite3_bind_int(st_r, 1, i) != SQLITE_OK
	    || sqlite3_bind_int(st_t, 1, i) != SQLITE_OK
	    || sq3_set_int64_default(st_r, 2, detector_rule_start_offset(r),
				     0) != SQLITE_OK
	    || sq3_set_int64_default(st_r, 3, detector_rule_end_offset(r),
				     DETECTOR_OFFSET_EOF) != SQLITE_OK
	    || sq3_set_int_default(st_r, 4, detector_rule_operation(r),
				   DETECTOR_OP_NONE) != SQLITE_OK
	    || sqlite3_step(st_r) != SQLITE_DONE
	    || sqlite3_reset(st_r) != SQLITE_OK)
	    return -1;

	for (j=0; j<detector_rule_num_tests(r); j++) {
	    t = detector_rule_test(r, j);

	    if (sqlite3_bind_int(st_t, 2, j) != SQLITE_OK
		|| (sqlite3_bind_int(st_t, 3, detector_test_type(t))
		    != SQLITE_OK)
		|| sqlite3_bind_int64(st_t, 4,
				      detector_test_offset(t)) != SQLITE_OK
		|| (sqlite3_bind_int(st_t, 8, detector_test_result(t))
		    != SQLITE_OK))
		return -1;

	    switch (detector_test_type(t)) {
	    case DETECTOR_TEST_DATA:
	    case DETECTOR_TEST_OR:
	    case DETECTOR_TEST_AND:
	    case DETECTOR_TEST_XOR:
		if (sqlite3_bind_null(st_t, 5) != SQLITE_OK
		    || sq3_set_blob(st_t, 6, detector_test_mask(t),
				    detector_test_length(t)) != SQLITE_OK
		    || sq3_set_blob(st_t, 7, detector_test_value(t),
				    detector_test_length(t)) != SQLITE_OK)
		    return -1;
		break;

	    case DETECTOR_TEST_FILE_EQ:
	    case DETECTOR_TEST_FILE_LE:
	    case DETECTOR_TEST_FILE_GR:
		if ((sqlite3_bind_int64(st_t, 5, detector_test_length(t))
		     != SQLITE_OK)
		    || sqlite3_bind_null(st_t, 6) != SQLITE_OK
		    || sqlite3_bind_null(st_t, 7) != SQLITE_OK)
		    return -1;
		break;
	    }

	    if (sqlite3_step(st_t) != SQLITE_DONE
		|| sqlite3_reset(st_t) != SQLITE_OK)
		return -1;
	}

    }

    return 0;
}
