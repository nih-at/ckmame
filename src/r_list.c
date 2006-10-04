/*
  $NiH: r_list.c,v 1.5 2006/04/15 22:52:58 dillo Exp $

  r_list.c -- read list struct from db
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



/* read list of strings from db */
#include <stdlib.h>
#include <string.h>

#include "dbh.h"
#include "r.h"
#include "xmalloc.h"



int
r_hashtypes(DB *db, int *romhashtypesp, int *diskhashtypesp)
{
    DBT v;
    void *data;

    if (dbh_lookup(db, DBH_KEY_HASH_TYPES, &v) != 0)
	return -1;

    data = v.data;

    *romhashtypesp = r__ushort(&v);
    *diskhashtypesp = r__ushort(&v);

    free(data);

    return 0;
}



parray_t *
r_list(DB *db, const char *key)
{
    DBT v;
    void *data;
    parray_t *pa;

    if (dbh_lookup(db, key, &v) != 0)
	return NULL;

    data = v.data;

    pa = r__parray(&v, (void *(*)())r__string);

    free(data);

    return pa;
}
