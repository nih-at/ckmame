/*
  $NiH: w_list.c,v 1.12 2004/04/24 09:40:25 dillo Exp $

  w_list.c -- write list struct to db
  Copyright (C) 1999, 2003, 2004 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

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



/* write list of strings to db */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "dbh.h"
#include "w.h"
#include "types.h"
#include "xmalloc.h"



int
w_hashtypes(DB *db, int romhashtypes, int diskhashtypes)
{
    int err;
    DBT v;

    v.data = NULL;
    v.size = 0;

    w__ushort(&v, romhashtypes);
    w__ushort(&v, diskhashtypes);

    err = ddb_insert(db, "/hashtypes", &v);

    free(v.data);

    return err;
}



int
w_list(DB *db, const char *key, const char * const *list, int n)
{
    int err;
    DBT v;

    v.data = NULL;
    v.size = 0;

    w__array(&v, w__pstring, list, sizeof(char *), n);

    err = ddb_insert(db, key, &v);

    free(v.data);

    return err;
}
