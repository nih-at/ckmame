/*
  $NiH: r_dat.c,v 1.3 2006/04/15 22:52:58 dillo Exp $

  r_dat.c -- read dat struct from db
  Copyright (C) 2006-2007 Dieter Baron and Thomas Klausner

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



#include <string.h>
#include <stdlib.h>

#include "dat.h"
#include "dbh.h"
#include "sq_util.h"
#include "xmalloc.h"

#define QUERY_DAT "select dat_idx, name, description, version from dat " \
		  "where dat_idx >= 0 order by dat_idx"



dat_t *
r_dat(sqlite3 *db)
{
    sqlite3_stmt *stmt;
    dat_t *dat;
    dat_entry_t *de;
    int ret;

    if (sqlite3_prepare_v2(db, QUERY_DAT, -1, &stmt, NULL) != SQLITE_OK) {
	/* XXX */
	return NULL;
    }

    dat = dat_new();

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	de = (dat_entry_t *)array_grow(dat, dat_entry_init);

	de->name = sq3_get_string(stmt, 0);
	de->description = sq3_get_string(stmt, 1);
	de->version = sq3_get_string(stmt, 2);
    }

    sqlite3_finalize(stmt);

    if (ret != SQLITE_DONE) {
	/* XXX */
	dat_free(dat);
	return NULL;
    }
    
    return dat;
}
