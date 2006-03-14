/*
  $NiH: check_archive.c,v 1.4 2005/10/05 21:21:33 dillo Exp $

  check_archive.c -- determine status of files in archive
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



#include "archive.h"
#include "find.h"
#include "funcs.h"
#include "globals.h"
#include "match.h"



file_status_array_t *
check_archive(archive_t *a, match_array_t *ma, const char *gamename)
{
    file_status_array_t *fsa;
    match_t *m, mr;
    int i;

    if (a == NULL)
	return NULL;

    fsa = file_status_array_new(archive_num_files(a));

    for (i=0; i<match_array_length(ma); i++) {
	m = match_array_get(ma, i);
	if (match_where(m) == ROM_INZIP && match_quality(m) != QU_HASHERR)
	    file_status_array_get(fsa, match_index(m))
		= match_quality(m) == QU_LONG ? FS_PARTUSED : FS_USED;
    }

    for (i=0; i<archive_num_files(a); i++) {
	if (rom_status(archive_file(a, i)) != STATUS_OK) {
	    file_status_array_get(fsa, i) = FS_BROKEN;
	    continue;
	}

	if (file_status_array_get(fsa, i) == FS_USED)
	    continue;

	switch (find_in_romset(db, archive_file(a, i), gamename, &mr)) {
	case FIND_UNKNOWN:
	    break;

	case FIND_EXISTS:
	    archive_free(match_archive(&mr));
	    file_status_array_get(fsa, i) = FS_SUPERFLUOUS;
	    break;

	case FIND_MISSING:
	    ensure_needed_maps();
	    if (find_in_archives(needed_file_map,
				 archive_file(a, i), &mr) == FIND_EXISTS) {
		archive_free(match_archive(&mr));
		file_status_array_get(fsa, i) = FS_SUPERFLUOUS;
	    }
	    else
		file_status_array_get(fsa, i) = FS_NEEDED;
	    break;

	case FIND_ERROR:
	    /* XXX: how to handle? */
	    break;
	}
    }
    
    return fsa;
}
