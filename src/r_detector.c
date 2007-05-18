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
#include "xmalloc.h"



detector_t *
r_detector(sqlite3 *db)
{
#if 0
    DBT v;
    detector_t *d;
    void *data;

    if (dbh_lookup(db, DBH_KEY_DETECTOR, &v) != 0)
	return NULL;

    data = v.data;

    d = xmalloc(sizeof(*d));
    
    d->name = r__string(&v);
    d->author = r__string(&v);
    d->version = r__string(&v);
    d->rules = r__array(&v, r__rule, sizeof(detector_rule_t));
    d->buf = NULL;
    d->buf_size = 0;
    
    free(data);

    return d;
#else
    return NULL;
#endif
}
