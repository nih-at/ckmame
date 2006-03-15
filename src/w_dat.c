/*
  $NiH: w_dat.c,v 1.1 2006/03/14 22:11:40 dillo Exp $

  w_dat.c -- write dat struct to db
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

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



#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "dat.h"
#include "dbh.h"
#include "w.h"
#include "types.h"
#include "xmalloc.h"

static void w__dat_entry(DBT *, const void *);



int
w_dat(DB *db, dat_t *dat)
{
    int err;
    DBT v;

    v.data = NULL;
    v.size = 0;

    w__array(&v, w__dat_entry, dat);

    err = ddb_insert(db, DDB_KEY_DAT, &v);

    free(v.data);

    return err;
}



static void
w__dat_entry(DBT *v, const void *vd)
{
    const dat_entry_t *d;

    d = (const dat_entry_t *)vd;

    w__string(v, dat_entry_name(d));
    w__string(v, dat_entry_version(d));
}
