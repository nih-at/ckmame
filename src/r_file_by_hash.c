/*
  $NiH: r_file_by_hash.c,v 1.3 2005/07/13 17:42:20 dillo Exp $

  r_file_by_hash.c -- read file_by_hash struct from db
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

#include "array.h"
#include "dbh.h"
#include "file_by_hash.h"
#include "xmalloc.h"
#include "r.h"

static void r__file_by_hash_entry(DBT *, void *);

array_t *
r_file_by_hash(DB *db, filetype_t ft, const hashes_t *hash)
{
    DBT v;
    array_t *a;
    void *data;

    if (ddb_lookup(db, file_by_hash_make_key(ft, hash), &v) != 0)
	return NULL;

    data = v.data;

    a = r__array(&v, r__file_by_hash_entry, sizeof(file_by_hash_t));
    
    free(data);

    return a;
}



static void
r__file_by_hash_entry(DBT *v, void *vr)
{
    file_by_hash_t *e;
    
    e = (file_by_hash_t *)vr;

    e->name = r__string(v);
    e->index = r__ushort(v);
}
