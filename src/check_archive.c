/*
  $NiH: check_archive.c,v 1.5 2006/03/14 20:30:35 dillo Exp $

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
    match_t *m, mr;
    int i;

    if (a == NULL)
	return;

    for (i=0; i<match_array_length(result_roms(res)); i++) {
	m = result_rom(res, i);
	if (match_where(m) == ROM_INZIP && match_quality(m) != QU_HASHERR)
	    result_file(res, match_index(m))
		= match_quality(m) == QU_LONG ? FS_PARTUSED : FS_USED;
    }

    for (i=0; i<archive_num_files(a); i++) {
	if (rom_status(archive_file(a, i)) != STATUS_OK) {
	    result_file(res, i) = FS_BROKEN;
	    continue;
	}

	if (result_file(res, i) == FS_USED)
	    continue;

	switch (find_in_romset(db, archive_file(a, i), gamename, &mr)) {
	case FIND_UNKNOWN:
	    break;

	case FIND_EXISTS:
	    archive_free(match_archive(&mr));
	    result_file(res, i) = FS_SUPERFLUOUS;
	    break;

	case FIND_MISSING:
	    ensure_needed_maps();
	    if (find_in_archives(needed_file_map,
				 archive_file(a, i), &mr) == FIND_EXISTS) {
		archive_free(match_archive(&mr));
		result_file(res, i) = FS_SUPERFLUOUS;
	    }
	    else
		result_file(res, i) = FS_NEEDED;
	    break;

	case FIND_ERROR:
	    /* XXX: how to handle? */
	    break;
	}
    }
}
