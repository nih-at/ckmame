/*
  $NiH: check_disks.c,v 1.6 2006/03/14 20:30:35 dillo Exp $

  check_disks.c -- match files against disks
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



#include "find.h"
#include "funcs.h"
#include "game.h"
#include "globals.h"
#include "match_disk.h"
#include "util.h"
#include "xmalloc.h"



void
check_disks(game_t *game, result_t *res)
{
    disk_t *d, *f;
    match_disk_t *md, mf;
    char *name;
    int i;

    if (game_num_disks(game) == 0)
	return;

    for (i=0; i<game_num_disks(game); i++) {
	md = result_disk(res, i);
	d = game_disk(game, i);
	
	name = findfile(disk_name(d), TYPE_DISK);
	if (name == NULL) {
	    f = NULL;
	    result_disk_file(res, i) = FS_MISSING;
	    result_disk_name(res, i) = NULL;
	}
	else {
	    f = disk_get_info(name, 0);
	    result_disk_name(res, i) = name;
	    
	    if (f == NULL) {
		match_disk_quality(md) = QU_MISSING;
		result_disk_file(res, i) = FS_BROKEN;
	    }
	}

	if (f) {
	    hashes_copy(match_disk_hashes(md), disk_hashes(f));
	    match_disk_name(md) = xstrdup(disk_name(f));
	    
	    switch (hashes_cmp(disk_hashes(d), disk_hashes(f))) {
	    case HASHES_CMP_NOCOMMON:
		match_disk_quality(md) = QU_NOHASH;
		result_disk_file(res, i) = FS_USED;
		break;
	    case HASHES_CMP_MATCH:
		match_disk_quality(md) = QU_OK;
		result_disk_file(res, i) = FS_USED;
		break;
	    case HASHES_CMP_MISMATCH:
		match_disk_quality(md)  = QU_HASHERR;
		switch (find_disk_in_romset(db, f, game_name(game), &mf)) {
		case FIND_UNKNOWN:
		    result_disk_file(res, i) = FS_UNKNOWN;
		    break;
		case FIND_MISSING:
		    result_disk_file(res, i) = FS_NEEDED;
		    break;
		case FIND_EXISTS:
		    result_disk_file(res, i) = FS_SUPERFLUOUS;
		    break;
		case FIND_ERROR:
		    /* XXX: how to handle? */
		    break;
		}
		break;
	    }
	
	    disk_free(f);
	}

	if (match_disk_quality(md) != QU_OK) {
	    /* search in needed */
	    ensure_needed_maps();
	    if (find_disk(needed_disk_map, d, md) == FIND_EXISTS)
		continue;
	    
	    /* search in superfluous and extra dirs */
	    ensure_extra_maps();
	    if (find_disk(extra_disk_map, d, md) == FIND_EXISTS)
		continue;
	}
    }
}
