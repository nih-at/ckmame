#ifndef _HAD_DBH_H
#define _HAD_DBH_H

/*
  $NiH: dbh.h,v 1.4 2005/09/27 21:33:02 dillo Exp $

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
#include "file_location.h"
#include "game.h"
#include "hashes.h"
#include "parray.h"
#include "types.h"



array_t *r_dat(DB *);
array_t *r_file_by_hash(DB *, filetype_t, const hashes_t *);
struct game *r_game(DB *, const char *);
int r_hashtypes(DB *, int *, int *);
parray_t *r_list(DB *, const char *);
int w_dat(DB *, array_t *);
int w_file_by_hash_parray(DB *, filetype_t, const hashes_t *, parray_t *);
int w_game(DB *, const game_t *);
int w_hashtypes(DB *, int, int);
int w_list(DB *, const char *, const parray_t *);

const char *filetype_db_key(filetype_t);

#endif /* dbh.h */
