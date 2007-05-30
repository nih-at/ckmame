/*
  $NiH: check_archive.c,v 1.10 2006/10/04 17:36:43 dillo Exp $

  check_archive.c -- determine status of files in archive
  Copyright (C) 2005-2006 Dieter Baron and Thomas Klausner

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



#include "archive.h"
#include "find.h"
#include "funcs.h"
#include "globals.h"
#include "result.h"



void
check_archive(archive_t *a, const char *gamename, result_t *res)
{
    int i;

    if (a == NULL)
	return;

    for (i=0; i<archive_num_files(a); i++) {
	if (rom_status(archive_file(a, i)) != STATUS_OK) {
	    result_file(res, i) = FS_BROKEN;
	    continue;
	}

	if (result_file(res, i) == FS_USED)
	    continue;

	if ((hashes_types(rom_hashes(archive_file(a, i))) & romhashtypes)
	    != romhashtypes) {
	    if (archive_file_compute_hashes(a, i, romhashtypes) < 0) {
		result_file(res, i) = FS_BROKEN;
		continue;
	    }
	}

	if (find_in_old(archive_file(a, i), NULL) == FIND_EXISTS) {
	    result_file(res, i) = FS_DUPLICATE;
	    continue;
	}

	switch (find_in_romset(archive_file(a, i), gamename, NULL)) {
	case FIND_UNKNOWN:
	    break;

	case FIND_EXISTS:
	    result_file(res, i) = FS_SUPERFLUOUS;
	    break;

	case FIND_MISSING:
	    if (rom_size(archive_file(a, i)) == 0)
		result_file(res, i) = FS_SUPERFLUOUS;
	    else {
		match_t m;
		ensure_needed_maps();
		match_init(&m);
		if (find_in_archives(archive_file(a, i), &m) != FIND_EXISTS)
		    result_file(res, i) = FS_NEEDED;
		else {
		    if (match_where(&m) == ROM_NEEDED)
			result_file(res, i) = FS_SUPERFLUOUS;
		    else
			result_file(res, i) = FS_NEEDED;
		}
		match_finalize(&m);
	    }
	    break;

	case FIND_ERROR:
	    /* XXX: how to handle? */
	    break;
	}
    }
}
