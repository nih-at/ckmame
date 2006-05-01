/*
  $NiH: check_disks.c,v 1.9 2006/04/26 21:01:51 dillo Exp $

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
check_images(images_t *im, const char *gamename, result_t *res)
{
    disk_t *d;
    int i;

    if (im == NULL)
	return;

    for (i=0; i<images_length(im); i++) {
	d = images_get(im, i);

	if (d == NULL) {
	    result_image(res, i) = FS_MISSING;
	    continue;
	}

	if (disk_status(d) != STATUS_OK) {
	    result_image(res, i) = FS_BROKEN;
	    continue;
	}

	if (result_image(res, i) == FS_USED)
	    continue;

	if ((hashes_types(disk_hashes(d)) & diskhashtypes) != diskhashtypes) {
	    /* XXX: compute missing hashes */
	}

	if (find_disk_in_old(d, NULL) == FIND_EXISTS) {
	    result_image(res, i) = FS_DUPLICATE;
	    continue;
	}

	switch (find_disk_in_romset(d, gamename, NULL)) {
	case FIND_UNKNOWN:
	    break;

	case FIND_EXISTS:
	    result_image(res, i) = FS_SUPERFLUOUS;
	    break;

	case FIND_MISSING:
	    ensure_needed_maps();
	    if (find_disk(needed_disk_map, d, NULL) != FIND_EXISTS)
		result_image(res, i) = FS_NEEDED;
	    else
		result_image(res, i) = FS_SUPERFLUOUS;
	    break;

	case FIND_ERROR:
	    /* XXX: how to handle? */
	    break;
	}
	
    }
}
