/*
  $NiH: w_prog.c,v 1.10 2005/06/20 16:16:04 wiz Exp $

  w_prog.c -- write prog struct to db
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
w_prog(DB *db, const char *name, const char *version)
{
    int err;
    DBT v;

    v.data = NULL;
    v.size = 0;

    w__string(&v, name);
    w__string(&v, version);

    err = ddb_insert(db, DDB_KEY_PROG, &v);

    free(v.data);

    return err;
}
