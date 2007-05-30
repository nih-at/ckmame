/*
  $NiH: check_disks.c,v 1.10 2006/05/01 21:09:11 dillo Exp $

  check_disks.c -- match files against disks
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



#include "find.h"
#include "funcs.h"
#include "game.h"
#include "globals.h"
#include "match_disk.h"
#include "util.h"
#include "xmalloc.h"



void
check_disks(game_t *game, images_t *im, result_t *res)
{
    disk_t *d, *f;
    match_disk_t *md;
    int i;

    if (game_num_disks(game) == 0)
	return;

    for (i=0; i<game_num_disks(game); i++) {
	md = result_disk(res, i);
	d = game_disk(game, i);
	f = images_get(im, i);

	if (match_disk_quality(md) == QU_OLD)
	    continue;

	if (f) {
	    match_disk_set_source(md, f);
	    
	    switch (hashes_cmp(disk_hashes(d), disk_hashes(f))) {
	    case HASHES_CMP_NOCOMMON:
		match_disk_quality(md) = QU_NOHASH;
		result_image(res, i) = FS_USED;
		break;
	    case HASHES_CMP_MATCH:
		match_disk_quality(md) = QU_OK;
		result_image(res, i) = FS_USED;
		break;
	    case HASHES_CMP_MISMATCH:
		match_disk_quality(md) = QU_HASHERR;
		break;
	    }
	}

	if (match_disk_quality(md) != QU_OK
	    && match_disk_quality(md) != QU_OLD) {
	    /* search in needed, superfluous and extra dirs */
	    ensure_extra_maps(DO_MAP);
	    ensure_needed_maps();
	    if (find_disk(d, md) == FIND_EXISTS)
		continue;
	}
    }
}
