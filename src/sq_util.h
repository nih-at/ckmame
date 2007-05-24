#ifndef HAD_SQ_UTIL_H
#define HAD_SQ_UTIL_H

/*
  $NiH$

  sq_util.h -- sqlite3 utility functions
  Copyright (C) 2007 Dieter Baron and Thomas Klausner

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

#include <sqlite3.h>

#include "myinttypes.h"



void *sq3_get_blob(sqlite3_stmt *, int, size_t *);
int sq3_get_int_default(sqlite3_stmt *, int, int);
int64_t sq3_get_int64_default(sqlite3_stmt *, int, int64_t);
int sq3_get_one_int(sqlite3 *, const char *, int *);
char *sq3_get_string(sqlite3_stmt *, int);
int sq3_set_blob(sqlite3_stmt *, int, const void *, size_t);
int sq3_set_int_default(sqlite3_stmt *, int, int, int);
int sq3_set_int64_default(sqlite3_stmt *, int, int64_t, int64_t);
int sq3_set_string(sqlite3_stmt *, int, const char *);

#endif /* sq_util.h */
