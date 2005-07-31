/*
  $NiH: check_disks.c,v 1.1.2.1 2005/07/27 00:05:57 dillo Exp $

  check_disks.c -- match files against disks
  Copyright (C) 1999, 2004, 2005 Dieter Baron and Thomas Klausner

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



#include "funcs.h"
#include "game.h"
#include "match_disk.h"
#include "util.h"
#include "xmalloc.h"



match_disk_array_t *
check_disks(game_t *game)
{
    match_disk_array_t *mda;
    match_disk_t *md;
    disk_t *d, *f;
    int i;

    if (game_num_disks(game) == 0)
	return NULL;

    mda = match_disk_array_new(game_num_disks(game));

    for (i=0; i<game_num_disks(game); i++) {
	md = match_disk_array_get(mda, i);
	d = game_disk(game, i);
	f = disk_get_info(findfile(disk_name(d), TYPE_DISK));

	if (f == NULL)
	    match_disk_quality(md) = QU_MISSING;
	else {
	    hashes_copy(match_disk_hashes(md), disk_hashes(f));
	    match_disk_name(md) = xstrdup(disk_name(f));
	
	    switch (hashes_cmp(disk_hashes(d), disk_hashes(f))) {
	    case HASHES_CMP_NOCOMMON:
		match_disk_quality(md) = QU_NOHASH;
		break;
	    case HASHES_CMP_MATCH:
		match_disk_quality(md) = QU_OK;
		break;
	    case HASHES_CMP_MISMATCH:
		match_disk_quality(md)  = QU_HASHERR;
		break;
	    }
	}

	disk_free(f);
    }

    return mda;
}
