#ifndef _HAD_DBH_H
#define _HAD_DBH_H

/*
  $NiH: dbh.h,v 1.1 2005/07/04 21:54:50 dillo Exp $

  dbh.h -- high level db functions
  Copyright (C) 1999, 2004 Dieter Baron and Thomas Klausner

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

#include "dbl.h"
#include "file_by_hash.h"
#include "hashes.h"
#include "parray.h"
#include "types.h"



struct game *r_game(DB *, const char *);
int r_hashtypes(DB *, int *, int *);
parray_t *r_list(DB *, const char *);
int r_prog(DB *, char **, char **);
file_by_hash_t *r_file_by_hash(DB *, filetype_t, const hashes_t *);
int w_file_by_hash_parray(DB *, filetype_t, const hashes_t *, parray_t *);
int w_game(DB *, const struct game *);
int w_hashtypes(DB *, int, int);
int w_list(DB *, const char *, const char * const *, int);
int w_prog(DB *, const char *, const char *);

#endif /* dbh.h */