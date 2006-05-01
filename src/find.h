#ifndef HAD_FIND_H
#define HAD_FIND_H

/*
  $NiH: find.h,v 1.6 2006/04/24 11:38:38 dillo Exp $

  find.h -- find ROM in ROM set or archives
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



#include "disk.h"
#include "map.h"
#include "match.h"
#include "match_disk.h"
#include "rom.h"

enum find_result {
    FIND_ERROR = -1,
    FIND_UNKNOWN,
    FIND_MISSING,
    FIND_EXISTS
};

typedef enum find_result find_result_t;



find_result_t find_disk(map_t *, const disk_t *, match_disk_t *);
find_result_t find_disk_in_old(const disk_t *, match_disk_t *);
find_result_t find_disk_in_romset(const disk_t *, const char *,
				  match_disk_t *);
find_result_t find_in_archives(map_t *, const rom_t *, match_t *);
find_result_t find_in_old(const rom_t *, match_t *);
find_result_t find_in_romset(const rom_t *, const char *, match_t *);

#endif /* find.h */
