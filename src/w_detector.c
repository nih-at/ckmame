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
#include "w.h"

static void w__rule(DBT *, const void *);
static void w__test(DBT *, const void *);



int
w_detector(DB *db, const detector_t *d)
{
    DBT v;
    int err;

    v.data = NULL;
    v.size = 0;

    w__string(&v, detector_name(d));
    w__string(&v, detector_author(d));
    w__string(&v, detector_version(d));
    w__array(&v, w__rule, detector_rules(d));

    err = dbh_insert(db, DBH_KEY_DETECTOR, &v);

    free(v.data);

    return err;
}



static void
w__rule(DBT *v, const void *vd)
{
    const detector_rule_t *dr;
    
    dr = vd;

    w__uint64(v, detector_rule_start_offset(dr));
    w__uint64(v, (uint64_t)detector_rule_end_offset(dr));
    w__uint8(v, (uint64_t)detector_rule_operation(dr));
    w__array(v, w__test, detector_rule_tests(dr));
}



static void
w__test(DBT *v, const void *vd)
{
    const detector_test_t *dt;
    
    dt = vd;

    w__uint8(v, detector_test_type(dt));
    w__uint64(v, (uint64_t)detector_test_offset(dt));
    w__uint64(v, detector_test_length(dt));
    if (detector_test_length(dt) > 0) {
	w__mem(v, detector_test_value(dt), detector_test_length(dt));
	if (detector_test_mask(dt)) {
	    w__uint8(v, true);
	    w__mem(v, detector_test_mask(dt), detector_test_length(dt));
	}
	else
	    w__uint8(v, false);
    }
    w__uint8(v, detector_test_result(dt));
}
