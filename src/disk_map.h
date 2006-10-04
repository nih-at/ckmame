#ifndef _HAD_DISK_MAP_H
#define _HAD_DISK_MAP_H

/*
  $NiH: disk_map.h,v 1.1 2006/04/17 11:31:11 dillo Exp $

  disk_map.h -- hash table of opened disk images
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

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



#include "disk.h"
#include "pmap.h"

typedef pmap_t disk_map_t;

#define disk_map_add(m, s, a)	(pmap_add((m), (s), (a)))
#define disk_map_delete(m, s, a)	(pmap_delete((m), (s), (a)))
#define disk_map_get(m, s)		((disk_t *)pmap_get((m), (s)))
#define disk_map_new()	\
	(pmap_new((pmap_free_f)disk_real_free))

#endif /* disk_map.h */
