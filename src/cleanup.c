/*
  $NiH: cleanup.c,v 1.12 2006/10/25 15:56:54 wiz Exp $

  cleanup.c -- clean up list of zip archives
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include "funcs.h"
#include "garbage.h"
#include "globals.h"
#include "util.h"
#include "warn.h"

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
	    /* XXX: where? */
	    if ((a=archive_new(name, TYPE_ROM, FILE_NOWHERE, 0)) == NULL) {
		/* XXX */
		continue;
	    }
	    res = result_new(NULL, a, NULL);

	    while (di < len) {
		fl = delete_list_get(del, di);
		cmp = strcmp(name, file_location_name(fl));
		
		if (cmp == 0)
		    result_file(res, file_location_index(fl)) = FS_USED;
		else if (cmp < 0)
		    break;

		di++;
	    }

	    check_archive(a, NULL, res);
	    
	    warn_set_info(WARN_TYPE_ARCHIVE, archive_name(a));
	    diagnostics_archive(a, res);
	    
	    cleanup_archive(a, res, flags);

	    result_free(res);
	    archive_close_zip(a);
	    archive_free(a);

	    break;

	case NAME_CHD:
	case NAME_NOEXT:
	    if ((im=images_new_name(name,
			    nt==NAME_NOEXT ? DISK_FL_QUIET : 0)) == NULL) {
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

	    check_images(im, NULL, res);
	    
	    warn_set_info(WARN_TYPE_IMAGE, name);
	    diagnostics_images(im, res);

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
    int i, move;
    char *reason;
    
    if ((flags & CLEANUP_UNKNOWN) && (fix_options & FIX_DO))
	gb = garbage_new(a);
	    
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
		       file_name(archive_file(a, i)));
	    archive_file_delete(a, i);
	    break;
	    
	case FS_BROKEN:
	case FS_MISSING:
	case FS_PARTUSED:
	    break;
	    
	case FS_NEEDED:
	    if (flags & CLEANUP_NEEDED) {
		if (fix_options & FIX_PRINT)
		    printf("%s: save needed file `%s'\n",
			   archive_name(a), file_name(archive_file(a, i)));
		/* XXX: handle error (how?) */
		save_needed(a, i, fix_options & FIX_DO);
	    }
	    break;
	    
	case FS_UNKNOWN:
	    if (flags & CLEANUP_UNKNOWN) {
		move = fix_options & FIX_MOVE_UNKNOWN;
		if (fix_options & FIX_PRINT)
		    printf("%s: %s unknown file `%s'\n",
			   archive_name(a),
			   (move ? "mv" : "delete"),
			   file_name(archive_file(a, i)));

		/* XXX: handle error (how?) */
		if (move)
		    garbage_add(gb, i, false);
		else
		    archive_file_delete(a, i);
	    }
	    break;
	}
    }
    
    if ((flags & CLEANUP_UNKNOWN) && (fix_options & FIX_DO)) {
	if (garbage_close(gb) < 0)
	    archive_rollback(a);
    }

    archive_commit(a);

    if (archive_is_empty(a))
	remove_empty_archive(archive_name(a));
}



static void
cleanup_disk(images_t *im, result_t *res, int flags)
{
    int i, move, ret;
    const char *name, *reason;
    
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
