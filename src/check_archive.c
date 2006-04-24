/*
  $NiH: check_archive.c,v 1.6 2006/04/05 22:36:03 dillo Exp $

  check_archive.c -- determine status of files in archive
  Copyright (C) 2005-2006 Dieter Baron and Thomas Klausner

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

	if (find_in_old(old_db, archive_file(a, i), NULL) == FIND_EXISTS) {
	    result_file(res, i) = FS_DUPLICATE;
	    continue;
	}

	switch (find_in_romset(db, archive_file(a, i), gamename, NULL)) {
	case FIND_UNKNOWN:
	    break;

	case FIND_EXISTS:
	    result_file(res, i) = FS_SUPERFLUOUS;
	    break;

	case FIND_MISSING:
	    ensure_needed_maps();
	    if (find_in_archives(needed_file_map,
				 archive_file(a, i), NULL) != FIND_EXISTS)
		result_file(res, i) = FS_NEEDED;
	    else
		result_file(res, i) = FS_SUPERFLUOUS;
	    break;

	case FIND_ERROR:
	    /* XXX: how to handle? */
	    break;
	}
    }
}
