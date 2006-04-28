/*
  $NiH: cleanup.c,v 1.2 2006/04/28 20:01:37 dillo Exp $

  cleanup.c -- clean up list of zip archives
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

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
#include "garbage.h"
#include "globals.h"



void
cleanup_list(parray_t *list, delete_list_t *del, int flags)
{
    archive_t *a;
    result_t *res;
    char *name;
    int i, j, di, len, cmp, keep;
    file_location_t *fl;
    char *reason;
    int survivors;
    garbage_t *gb;

    di = len = 0;
    if (del) {
	delete_list_sort(del);
	len = delete_list_length(del);
    }

    for (i=0; i<parray_length(list); i++) {
	name = (char *)parray_get(list, i);
	if ((a=archive_new(name, TYPE_FULL_PATH, 0)) == NULL) {
	    /* XXX */
	    continue;
	}
	res = result_new(NULL, a);
	if (flags & CLEANUP_UNKNOWN && fix_options & FIX_DO)
	    gb = garbage_new(a);

	while (di < len) {
	    fl = delete_list_get(del, di++);
	    cmp = strcmp(name, file_location_name(fl));

	    if (cmp == 0)
		result_file(res, file_location_index(fl)) = FS_USED;
	    else if (cmp < 0)
		break;
	}

	check_archive(a, NULL, res);

	archive_ensure_zip(a, 0);
	survivors = 0;

	for (j=0; j<archive_num_files(a); j++) {
	    switch (result_file(res, j)) {
	    case FS_SUPERFLUOUS:
	    case FS_DUPLICATE:
	    case FS_USED:
		switch (result_file(res, j)) {
		case FS_SUPERFLUOUS:
		    reason = "unused";
		    break;
		case FS_DUPLICATE:
		    reason = "duplicate";
		    break;
		case FS_USED:
		    reason = "used";
		    break;
		default:
		    reason = "[internal error]";
		    break;
		}
		
		if (fix_options & FIX_PRINT)
		    printf("%s: delete %s file `%s'\n",
			   archive_name(a), reason,
			   rom_name(archive_file(a, j)));
		if (fix_options & FIX_DO)
		    zip_delete(archive_zip(a), j);
		break;

	    case FS_BROKEN:
	    case FS_MISSING:
	    case FS_PARTUSED:
		survivors = 1;
		break;

	    case FS_NEEDED:
		if (flags & CLEANUP_NEEDED) {
		    if (fix_options & FIX_PRINT)
			printf("%s: save needed file `%s'\n",
			       archive_name(a), rom_name(archive_file(a, j)));
		    if (fix_options & FIX_DO) {
			if (save_needed(a, j, 1) != -1)
			    zip_delete(archive_zip(a), j);
			else
			    survivors = 1;
		    }
		}
		else
		    survivors = 1;
		break;
		
	    case FS_UNKNOWN:
		if (flags & CLEANUP_NEEDED) {
		    keep = fix_options & FIX_KEEP_UNKNOWN;
		    if (fix_options & FIX_PRINT)
			printf("%s: %s unknown file `%s'\n",
			       archive_name(a),
			       (keep ? "mv" : "delete"),
			       rom_name(archive_file(a, j)));
		    if (fix_options & FIX_DO) {
			if (keep)
			    keep = (garbage_add(gb, j) == -1);
			if (keep == 0) {
			    zip_delete(archive_zip(a), j);
			}
			else
			    survivors = 1;
		    }
		    else
			survivors = 1;
		}
		else
		    survivors = 1;
		break;
	    }
	}

	if (flags & CLEANUP_UNKNOWN && fix_options & FIX_DO) {
	    if (garbage_close(gb) < 0) {
		for (j=0; j<archive_num_files(a); j++) {
		    if (result_file(res, j) == FS_UNKNOWN)
			zip_unchange(archive_zip(a), j);
		}
	    }
	}

	archive_close_zip(a);
	archive_free(a);
	result_free(res);

	if ((fix_options & FIX_DO) && !survivors)
	    remove_empty_archive(name);
    }
}
