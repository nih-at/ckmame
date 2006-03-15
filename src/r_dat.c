/*
  $NiH: r_dat.c,v 1.1 2006/03/14 22:11:40 dillo Exp $

  r_dat.c -- read dat struct from db
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



#include <string.h>
#include <stdlib.h>

#include "dat.h"
#include "dbh.h"
#include "r.h"
#include "xmalloc.h"

static void r__dat_entry(DBT *, void *);



dat_t *
r_dat(DB *db)
{
    DBT v;
    void *data;
    array_t *dat;

    if (ddb_lookup(db, DDB_KEY_DAT, &v) != 0)
	return NULL;
    
    data = v.data;

    dat = r__array(&v, r__dat_entry, sizeof(dat_entry_t));
    
    free(data);

    return dat;
}



static void
r__dat_entry(DBT *v, void *vd)
{
    dat_entry_t *d;
    
    d = (dat_entry_t *)vd;

    dat_entry_name(d) = r__string(v);
    dat_entry_version(d) = r__string(v);
}
