#ifndef _HAD_ARCHIVE_MAP_H
#define _HAD_ARCHIVE_MAP_H

/*
  $NiH$

  archive_map.h -- hash table of opened archvies
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



#include "archive.h"
#include "pmap.h"

typedef pmap_t archive_map_t;

#define archive_map_add(m, s, a)	(pmap_add((m), (s), (a)))
#define archive_map_delete(m, s, a)	(pmap_delete((m), (s), (a)))
#define archive_map_get(m, s)		((archive_t *)pmap_get((m), (s)))
#define archive_map_new()	\
	(pmap_new((pmap_free_f)archive_real_free))

#endif /* archive_map.h */
