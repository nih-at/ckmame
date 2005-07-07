/*
  $NiH: w_file_by_hash.c,v 1.1 2005/07/04 21:54:51 dillo Exp $

  w_file_by_hash.c -- write file_by_hash struct to db
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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



#include <stdlib.h>
#include <string.h>

#include "dbh.h"
#include "error.h"
#include "parray.h"
#include "romutil.h"
#include "types.h"
#include "util.h"
#include "w.h"
#include "xmalloc.h"

static int file_by_hash_entry_cmp(const void *, const void *);
static void w__file_by_hash_entry(DBT *, const void *);



int
w_file_by_hash_parray(DB *db, filetype_t ft, const hashes_t *h, parray_t *pa)
{
    int err;
    DBT v;

    v.data = NULL;
    v.size = 0;

    if (parray_length(pa) == 0) {
	myerror(ERRDEF, "internal error: empty file_by_hash structure");
	return -1;
    }
    parray_sort(pa, file_by_hash_entry_cmp);

    w__parray(&v, w__file_by_hash_entry, pa);

    err = ddb_insert(db, file_by_hash_make_key(ft, h), &v);

    free(v.data);

    return err;
}



static void
w__file_by_hash_entry(DBT *v, const void *vr)
{
    const file_by_hash_entry_t *fbh;

    fbh = (const file_by_hash_entry_t *)vr;

    w__string(v, fbh->game);
    w__ushort(v, fbh->index);
}



static int
file_by_hash_entry_cmp(const void *a, const void *b)
{
    return strcasecmp(((const file_by_hash_entry_t *)a)->game,
		      ((const file_by_hash_entry_t *)b)->game);
}
