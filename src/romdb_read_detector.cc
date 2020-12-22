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

static bool romdb_read_rules(detector_t *, sqlite3_stmt *, sqlite3_stmt *);


DetectorPtr
romdb_read_detector(romdb_t *db) {
    sqlite3_stmt *stmt, *stmt2;

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_DAT_DETECTOR)) == NULL) {
	return NULL;
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
	return NULL;
    }

    auto detector = std::make_shared<Detector>();

    detector->name = sq3_get_string(stmt, 0);
    detector->author = sq3_get_string(stmt, 1);
    detector->version = sq3_get_string(stmt, 2);

    if ((stmt = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_RULE)) == NULL || (stmt2 = dbh_get_statement(romdb_dbh(db), DBH_STMT_QUERY_TEST)) == NULL) {
	return NULL;
    }

    if (!romdb_read_rules(detector.get(), stmt, stmt2)) {
	return NULL;
    }

    return detector;
}


static bool
romdb_read_rules(Detector *detector, sqlite3_stmt *st_r, sqlite3_stmt *st_t) {
    int ret;
    while ((ret = sqlite3_step(st_r)) == SQLITE_ROW) {
        Detector::Rule rule;

        auto idx = sqlite3_column_int(st_r, 0);
	rule.start_offset = sq3_get_int64_default(st_r, 1, 0);
	rule.end_offset = sq3_get_int64_default(st_r, 2, DETECTOR_OFFSET_EOF);
        rule.operation = static_cast<Detector::Operation>(sq3_get_int_default(st_r, 3, Detector::OP_NONE));

        if (sqlite3_bind_int(st_t, 1, idx) != SQLITE_OK) {
	    return false;
        }

	while ((ret = sqlite3_step(st_t)) == SQLITE_ROW) {
            Detector::Test test;

            test.type = static_cast<Detector::TestType>(sqlite3_column_int(st_t, 0));
	    test.offset = sqlite3_column_int64(st_t, 1);
	    test.result = sqlite3_column_int64(st_t, 5);

	    switch (test.type) {
                case Detector::TEST_DATA:
                case Detector::TEST_OR:
                case Detector::TEST_AND:
                case Detector::TEST_XOR:
                    test.mask = sq3_get_blob(st_t, 3);
                    test.value = sq3_get_blob(st_t, 4);
                    if (!test.mask.empty() && test.mask.size() != test.value.size()) {
                        return false;
                    }
                    test.length = test.mask.size();
                    break;
                    
                case Detector::TEST_FILE_EQ:
                case Detector::TEST_FILE_LE:
                case Detector::TEST_FILE_GR:
                    test.length = static_cast<uint64_t>(sqlite3_column_int64(st_t, 2));
                    break;
            }
            
            rule.tests.push_back(test);
	}
	if (ret != SQLITE_DONE || sqlite3_reset(st_t) != SQLITE_OK)
	    return -1;
        
        detector->rules.push_back(rule);
    }

    return 0;
}
