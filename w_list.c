/*
  w_list.c -- write list strcut to db
  Copyright (C) 1999 Dieter Baron and Thomas Klaunser

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

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
w_list(DB *db, char *key, char **list, int n)
{
    int err;
    DBT k, v;

    k.size = strlen(key);
    k.data = xmalloc(k.size);
    strncpy(k.data, key, k.size);

    v.data = NULL;
    v.size = 0;

    w__array(&v, w__pstring, list, sizeof(char *), n);

    err = ddb_insert(db, &k, &v);

    free(k.data);
    free(v.data);

    return err;
}
