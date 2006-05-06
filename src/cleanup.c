/*
  $NiH: cleanup.c,v 1.6 2006/05/05 00:44:45 wiz Exp $

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

static void cleanup_archive(archive_t *, result_t *, int);
static void cleanup_disk(images_t *, result_t *, int);



void
cleanup_list(parray_t *list, delete_list_t *del, int flags)
{
    archive_t *a;
    images_t *im;
    result_t *res;
    char *name;
    int i, di, len, cmp, n;
    file_location_t *fl;
    name_type_t nt;

    di = len = 0;
    if (del) {
	delete_list_sort(del);
	len = delete_list_length(del);
    }

    n = parray_length(list);
    i = 0;
    while (i < n) {
	name = (char *)parray_get(list, i);
	switch ((nt=name_type(name))) {
	case NAME_ZIP:
	    if ((a=archive_new(name, TYPE_FULL_PATH, 0)) == NULL) {
		/* XXX */
		continue;
	    }
	    res = result_new(NULL, a, NULL);

	    while (di < len) {
		fl = delete_list_get(del, di++);
		cmp = strcmp(name, file_location_name(fl));
		
		if (cmp == 0)
		    result_file(res, file_location_index(fl)) = FS_USED;
		else if (cmp < 0)
		    break;
	    }

	    cleanup_archive(a, res, flags);

	    result_free(res);
	    archive_close_zip(a);
	    archive_free(a);

	    break;

	case NAME_CHD:
	case NAME_NOEXT:
	    if ((im=images_new_name(name, nt==NAME_NOEXT)) == NULL) {
		/* XXX */
		continue;
	    }

	    res = result_new(NULL, NULL, im);

	    while (di < len) {
		fl = delete_list_get(del, di++);
		cmp = strcmp(name, file_location_name(fl));
		
		if (cmp == 0)
		    result_image(res, i) = FS_USED;
		else if (cmp < 0)
		    break;
	    }

	    cleanup_disk(im, res, flags);

	    result_free(res);
	    images_free(im);

	case NAME_UNKNOWN:
	    /* unknown files shouldn't be in list */
	    break;
	}

	if (n != parray_length(list))
	    n = parray_length(list);
	else
	    i++;
    }
}



static void
cleanup_archive(archive_t *a, result_t *res, int flags)
{
    garbage_t *gb;
    int i, move, survivors;
    char *reason;
    
    if (flags & CLEANUP_UNKNOWN && fix_options & FIX_DO)
	gb = garbage_new(a);
	    
    check_archive(a, NULL, res);
	    
    archive_ensure_zip(a, 0);
    survivors = 0;
	    
    for (i=0; i<archive_num_files(a); i++) {
	switch (result_file(res, i)) {
	case FS_SUPERFLUOUS:
	case FS_DUPLICATE:
	case FS_USED:
	    switch (result_file(res, i)) {
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
		       rom_name(archive_file(a, i)));
	    if (fix_options & FIX_DO)
		zip_delete(archive_zip(a), i);
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
			   archive_name(a), rom_name(archive_file(a, i)));
		if (fix_options & FIX_DO) {
		    if (save_needed(a, i, 1) != -1)
			zip_delete(archive_zip(a), i);
		    else
			survivors = 1;
		}
	    }
	    else
		survivors = 1;
	    break;
	    
	case FS_UNKNOWN:
	    if (flags & CLEANUP_UNKNOWN) {
		move = fix_options & FIX_MOVE_UNKNOWN;
		if (fix_options & FIX_PRINT)
		    printf("%s: %s unknown file `%s'\n",
			   archive_name(a),
			   (move ? "mv" : "delete"),
			   rom_name(archive_file(a, i)));
		if (fix_options & FIX_DO) {
		    if (move)
			move = (garbage_add(gb, i) == -1);
		    if (move == 0) {
			zip_delete(archive_zip(a), i);
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
	    for (i=0; i<archive_num_files(a); i++) {
		if (result_file(res, i) == FS_UNKNOWN)
		    zip_unchange(archive_zip(a), i);
	    }
	}
    }

    if ((fix_options & FIX_DO) && !survivors)
	remove_empty_archive(archive_name(a));
}



static void
cleanup_disk(images_t *im, result_t *res, int flags)
{
    int i, move, ret;
    const char *name, *reason;
    
    check_images(im, NULL, res);
	    
    for (i=0; i<images_length(im); i++) {
	if ((name=images_name(im, i)) == NULL)
	    continue;

	switch (result_image(res, i)) {
	case FS_SUPERFLUOUS:
	case FS_DUPLICATE:
	case FS_USED:
	    switch (result_image(res, i)) {
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
		printf("%s: delete %s image\n", name, reason);
	    if (fix_options & FIX_DO) {
		if (my_remove(name) == 0)
		    remove_from_superfluous(name);
	    }		
	    break;
	    
	case FS_BROKEN:
	case FS_MISSING:
	case FS_PARTUSED:
	    break;
	    
	case FS_NEEDED:
	    if (flags & CLEANUP_NEEDED) {
		if (fix_options & FIX_PRINT)
		    printf("%s: save needed image\n", name);
		save_needed_disk(name, (fix_options & FIX_DO));
		if (fix_options & FIX_DO)
		    remove_from_superfluous(name);
	    }
	    break;
	    
	case FS_UNKNOWN:
	    if (flags & CLEANUP_UNKNOWN) {
		move = fix_options & FIX_MOVE_UNKNOWN;
		if (fix_options & FIX_PRINT)
		    printf("%s: %s unknown image\n",
			   name, (move ? "mv" : "delete"));
		if (fix_options & FIX_DO) {
		    if (move)
			ret = move_image_to_garbage(name);
		    else
			ret = my_remove(name);
		    if (ret == 0)
			remove_from_superfluous(name);
		}
	    }
	    break;
	}
    }
}
