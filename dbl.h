#ifndef _HAD_DBL_H
#define _HAD_DBL_H

/*
  $NiH$

  dbl.h -- generic low level data base routines
  Copyright (C) 1999 Dieter Baron and Thomas Klaunser

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include "dbl-int.h"

DB* ddb_open(char *name, int extp, int writep);
int ddb_close(DB* db);
int ddb_insert(DB* db, DBT* key, DBT* value);	/* compressing versions */
int ddb_lookup(DB* db, DBT* key, DBT* value);

int ddb_insert_l(DB* db, DBT* key, DBT* value);	/* non-compressing versions */
int ddb_lookup_l(DB* db, DBT* key, DBT* value);

const char *ddb_error(void);
char *ddb_name(char *prefix);

#endif
