#ifndef _HAD_DBL_H
#define _HAD_DBL_H

/*
  $NiH: dbl.h,v 1.15 2004/02/26 02:26:08 wiz Exp $

  dbl.h -- generic low level data base routines
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



#include "dbl-int.h"

#define DDB_READ	0x0	/* open readonly */
#define DDB_WRITE	0x1	/* open for writing */
#define DDB_EXT		0x2	/* append extension to filename */

#define DDB_FORMAT_VERSION	3 /* version of ckmame database format */

DB* ddb_open(const char *name, int flags);
int ddb_check_version(DB *db, int flags);
int ddb_init_db(DB *db);
int ddb_close(DB *db);
int ddb_insert(DB *db, const char *key, DBT *value);	/* API versions */
int ddb_lookup(DB *db, const char *key, DBT *value);
const char *ddb_error(void);
char *ddb_name(const char *prefix);

int ddb_insert_l(DB *db, DBT *key, DBT *value);	/* backend versions */
int ddb_lookup_l(DB *db, DBT *key, DBT *value);
const char *ddb_error_l(void);


#endif
