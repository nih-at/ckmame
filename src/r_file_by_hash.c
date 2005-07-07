/*
  $NiH: r_file_by_hash.c,v 1.1 2005/07/04 21:54:51 dillo Exp $

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

#include "types.h"
#include "dbh.h"
#include "xmalloc.h"
#include "r.h"
#include "romutil.h"

static void r__file_by_hash_entry(DBT *, void *);

file_by_hash_t *
r_file_by_hash(DB *db, filetype_t ft, const hashes_t *hash)
{
    DBT v;
    struct file_by_hash *fbh;
    void *data;

    if (ddb_lookup(db, file_by_hash_make_key(ft, hash), &v) != 0)
	return NULL;

    data = v.data;

    fbh = file_by_hash_new();
    
    fbh->nentry = r__array(&v, r__file_by_hash_entry, (void *)&fbh->entry,
			   sizeof(fbh->entry[0]));
    
    free(data);

    return fbh;
}



static void
r__file_by_hash_entry(DBT *v, void *vr)
{
    file_by_hash_entry_t *e;
    
    e = (file_by_hash_entry_t *)vr;

    e->game = r__string(v);
    e->index = r__ushort(v);
}
