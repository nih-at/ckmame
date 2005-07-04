/*
  $NiH: w_file_by_hash.c,v 1.1 2005/06/20 16:16:04 wiz Exp $

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

#include "types.h"
#include "dbh.h"
#include "error.h"
#include "util.h"
#include "xmalloc.h"
#include "w.h"
#include "romutil.h"

static int fbh_entry_compare(const void *, const void *);
static void w__file_by_hash_entry(DBT *, const void *);



int
w_file_by_hash(DB *db, const struct file_by_hash *fbh)
{
    int err;
    DBT v;
    char *key;

    v.data = NULL;
    v.size = 0;

    if (fbh->nentry == 0) {
	myerror(ERRDEF, "internal error: empty file_by_hash structure");
	return -1;
    }

    qsort(fbh->entry, fbh->nentry, sizeof(fbh->entry[0]),
	  fbh_entry_compare);

    w__array(&v, w__file_by_hash_entry, fbh->entry,
	     sizeof(fbh->entry[0]), fbh->nentry);

    key = file_by_hash_make_key(fbh->filetype, &fbh->hash);

    err = ddb_insert(db, key, &v);

    free(v.data);
    free(key);

    return err;
}



static void
w__file_by_hash_entry(DBT *v, const void *vr)
{
    const struct file_by_hash_entry *e;

    e = (const struct file_by_hash_entry *)vr;

    w__string(v, e->game);
    w__ushort(v, e->index);
}



static int
fbh_entry_compare(const void *a, const void *b)
{
    return strcasecmp(((const struct file_by_hash_entry *)a)->game,
		      ((const struct file_by_hash_entry *)b)->game);
}
