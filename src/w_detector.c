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



int
w_detector(sqlite3 *db, const detector_t *d)
{
#if 0
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
#else
    return 0;
#endif
}
