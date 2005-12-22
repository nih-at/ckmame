/*
  $NiH: check_disks.c,v 1.4 2005/10/05 21:21:33 dillo Exp $

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



#include "find.h"
#include "funcs.h"
#include "game.h"
#include "globals.h"
#include "match_disk.h"
#include "util.h"
#include "xmalloc.h"



int
check_disks(game_t *game, file_status_array_t **dsap,
	    match_disk_array_t **mdap, parray_t **dnp)
{
    match_disk_array_t *mda;
    match_disk_t *md, mf;
    file_status_array_t *dsa;
    parray_t *dn;
    disk_t *d, *f;
    char *name;
    int i;

    if (game_num_disks(game) == 0) {
	*dsap = NULL;
	*mdap = NULL;
	*dnp = NULL;
	return 0;
    }

    mda = match_disk_array_new(game_num_disks(game));
    dsa = file_status_array_new(game_num_disks(game));
    dn = parray_new_sized(game_num_disks(game));

    for (i=0; i<game_num_disks(game); i++) {
	md = match_disk_array_get(mda, i);
	d = game_disk(game, i);
	
	name = findfile(disk_name(d), TYPE_DISK);
	if (name == NULL) {
	    f = NULL;
	    file_status_array_get(dsa, i) = FS_MISSING;
	    parray_push(dn, NULL);
	}
	else {
	    f = disk_get_info(name, 0);
	    parray_push(dn, name);
	    
	    if (f == NULL) {
		match_disk_quality(md) = QU_MISSING;
		file_status_array_get(dsa, i) = FS_BROKEN;
	    }
	}

	if (f) {
	    hashes_copy(match_disk_hashes(md), disk_hashes(f));
	    match_disk_name(md) = xstrdup(disk_name(f));
	    
	    switch (hashes_cmp(disk_hashes(d), disk_hashes(f))) {
	    case HASHES_CMP_NOCOMMON:
		match_disk_quality(md) = QU_NOHASH;
		file_status_array_get(dsa, i) = FS_USED;
		break;
	    case HASHES_CMP_MATCH:
		match_disk_quality(md) = QU_OK;
		file_status_array_get(dsa, i) = FS_USED;
		break;
	    case HASHES_CMP_MISMATCH:
		match_disk_quality(md)  = QU_HASHERR;
		switch (find_disk_in_romset(f, game_name(game), &mf)) {
		case FIND_UNKNOWN:
		    file_status_array_get(dsa, i) = FS_UNKNOWN;
		    break;
		case FIND_MISSING:
		    file_status_array_get(dsa, i) = FS_NEEDED;
		    break;
		case FIND_EXISTS:
		    file_status_array_get(dsa, i) = FS_SUPERFLUOUS;
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

    *mdap = mda;
    *dsap = dsa;
    *dnp = dn;
    return 0;
}
