/*
  $NiH: w_dat.c,v 1.3 2006/04/15 22:52:58 dillo Exp $

  w_dat.c -- write dat struct to db
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



#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "dat.h"
#include "dbh.h"
#include "sq_util.h"
#include "types.h"

#define DELETE_DAT	"delete from dat where dat_idx >= 0"
#define INSERT_DAT	"insert into dat (dat_idx, name, description, " \
			"version) values (?, ?, ?, ?)"



int
w_dat(sqlite3 *db, dat_t *d)
{
    sqlite3_stmt *stmt;
    int i;

    if (sqlite3_prepare_v2(db, INSERT_DAT, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    for (i=0; i<dat_length(d); i++) {
	if (sqlite3_bind_int(stmt, 1, i) != SQLITE_OK
	    || sq3_set_string(stmt, 2, dat_name(d, i)) != SQLITE_OK
	    || sq3_set_string(stmt, 3, dat_description(d, i)) != SQLITE_OK
	    || sq3_set_string(stmt, 4, dat_version(d, i)) != SQLITE_OK
	    || sqlite3_step(stmt) != SQLITE_DONE
	    || sqlite3_reset(stmt) != SQLITE_OK) {
	    sqlite3_finalize(stmt);
	    return -1;
	}
    }

    sqlite3_finalize(stmt);

    return 0;
}
