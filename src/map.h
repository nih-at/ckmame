#ifndef _HAD_MAP_H
#define _HAD_MAP_H

/*
  $NiH$

  map.h -- hash table
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



#include "dbl.h"
#include "hashes.h"
#include "parray.h"

typedef DB map_t;

#define MAP_FREE_FN(fn)	((void (*)(void *))fn)



int map_add(map_t *, int, const hashes_t *, void *);
int map_foreach(map_t *, int (*)(const hashes_t *, parray_t *, void *),
		void *);
void map_free(map_t *, void (*)(void *));
parray_t *map_get(map_t *, int, const hashes_t *);
map_t *map_new(void);

#endif /* map.h */
