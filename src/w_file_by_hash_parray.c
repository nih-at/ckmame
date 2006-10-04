/*
  $NiH: w_file_by_hash_parray.c,v 1.4 2006/04/15 22:52:58 dillo Exp $

  w_file_location.c -- write file_by_hash information to db
  Copyright (C) 2005-2006 Dieter Baron and Thomas Klausner

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
#include <string.h>

#include "dbh.h"
#include "error.h"
#include "parray.h"
#include "types.h"
#include "util.h"
#include "w.h"
#include "xmalloc.h"

static void w__file_location(DBT *, const void *);



int
w_file_by_hash_parray(DB *db, filetype_t ft, const hashes_t *h, parray_t *pa)
{
    int err;
    DBT v;

    v.data = NULL;
    v.size = 0;

    if (parray_length(pa) == 0) {
	myerror(ERRDEF, "internal error: empty file_location structure");
	return -1;
    }

    w__parray(&v, w__file_location, pa);

    err = dbh_insert(db, file_location_make_key(ft, h), &v);

    free(v.data);

    return err;
}



static void
w__file_location(DBT *v, const void *vr)
{
    const file_location_t *fbh;

    fbh = (const file_location_t *)vr;

    w__string(v, fbh->name);
    w__ushort(v, fbh->index);
}
