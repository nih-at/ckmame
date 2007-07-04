/*
  $NiH: fix.c,v 1.29 2006/10/04 17:36:44 dillo Exp $

  fix.c -- fix ROM sets
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <zip.h>

#include "archive.h"
#include "error.h"
#include "file_location.h"
#include "funcs.h"
#include "game.h"
#include "garbage.h"
#include "globals.h"
#include "match.h"
#include "pri.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

static int fix_disks(game_t *, images_t *, result_t *res);
static int fix_files(game_t *, archive_t *, result_t *);



int
fix_game(game_t *g, archive_t *a, images_t *im, result_t *res)
{
    int i, islong, move, archive_changed;
    int *deleted;
    garbage_t *gb;

    archive_changed = 0;

    if (fix_options & FIX_DO) {
	gb = garbage_new(a);
	
	if (archive_ensure_zip(a, 1) < 0) {
	    char *new_name;

	    /* opening the zip file failed, rename it and create new one */

	    if ((new_name=make_unique_name("broken", "%s",
					   archive_name(a))) == NULL)
		return -1;

	    if (fix_options & FIX_PRINT)
		printf("%s: rename broken zip to `%s'\n",
		       archive_name(a), new_name);
	    if (rename_or_move(archive_name(a), new_name) < 0) {
		free(new_name);
		return -1;
	    }
	    free(new_name);
	    if (archive_ensure_zip(a, 1) < 0)
		return -1;
	}

	deleted = malloc(sizeof(int) * archive_num_files(a));
	for (i=0; i<archive_num_files(a); i++)
	    deleted[i] = 0;

	if (extra_delete_list)
	    delete_list_mark(extra_delete_list);
	if (needed_delete_list)
	    delete_list_mark(needed_delete_list);
	if (superfluous_delete_list)
	    delete_list_mark(superfluous_delete_list);
    }

    for (i=0; i<archive_num_files(a); i++) {
	switch (result_file(res, i)) {
	case FS_UNKNOWN:
	    if (fix_options & FIX_IGNORE_UNKNOWN)
		break;
	    /* FALLTHROUGH */
	case FS_PARTUSED:
	    islong = (result_file(res, i) == FS_PARTUSED);
	    move = fix_options & (islong ? FIX_MOVE_UNKNOWN : FIX_MOVE_LONG);
	    if (fix_options & FIX_PRINT)
		printf("%s: %s %s file `%s'\n",
		       archive_name(a),
		       (move ? "mv" : "delete"),
		       (islong ? "long" : "unknown"),
		       rom_name(archive_file(a, i)));
	    if (fix_options & FIX_DO) {
		if (move)
		    move = (garbage_add(gb, i) == -1);
		if (move == 0) {
		    archive_changed = 1;
		    deleted[i] = 1;
		    zip_delete(archive_zip(a), i);
		}
	    }
	    break;

	case FS_DUPLICATE:
	    if (!(fix_options & FIX_DELETE_DUPLICATE))
		break;
	    /* fallthrough */
	case FS_SUPERFLUOUS:
	    if (fix_options & FIX_PRINT)
		printf("%s: delete %s file `%s'\n",
		       archive_name(a),
		       (result_file(res, i) == FS_SUPERFLUOUS
			? "unused" : "duplicate"),
		       rom_name(archive_file(a, i)));
	    if (fix_options & FIX_DO) {
		archive_changed = 1;
		zip_delete(archive_zip(a), i);
	    }
	    break;

	case FS_NEEDED:
	    if (fix_options & FIX_PRINT)
		printf("%s: save needed file `%s'\n",
		       archive_name(a),
		       rom_name(archive_file(a, i)));
	    if (save_needed(a, i, fix_options & FIX_DO) != -1) {
		if (fix_options & FIX_DO) {
		    archive_changed = 1;
		    zip_delete(archive_zip(a), i);
		}
	    }
	    break;

	case FS_BROKEN:
	case FS_USED:
	case FS_MISSING:
	    /* nothing to be done */
	    break;
	}
    }

    if (fix_options & FIX_DO) {
	if (garbage_close(gb) < 0) {
	    /* undelete files we tried to move to garbage */
	    for (i=0; i<archive_num_files(a); i++) {
		if (deleted[i]) {
		    if (zip_unchange(archive_zip(a), i) < 0) {
			/* XXX: cannot undelete */
		    }
		}
	    }
	}
	free(deleted);
    }
    
    if ((fix_options & (FIX_DO|FIX_PRINT)) == 0) {
	/* return early if no further messages or work requested */
	return 0;
    }

    archive_changed |= fix_files(g, a, res);

    if (archive_close_zip(a) < 0) {
	archive_changed = 0;
	if ((fix_options & FIX_DO) && extra_delete_list)
	    delete_list_rollback(extra_delete_list);
	if ((fix_options & FIX_DO) && needed_delete_list)
	    delete_list_rollback(needed_delete_list);
	if ((fix_options & FIX_DO) && superfluous_delete_list)
	    delete_list_rollback(superfluous_delete_list);
    }

    fix_disks(g, im, res);

    if (archive_changed)
	archive_refresh(a);

    return archive_changed;
}



static int
fix_disks(game_t *g, images_t *im, result_t *res)
{
    int i;
    disk_t *d;
    match_disk_t *md;
    const char *name;
    char *fname;
    
    for (i=0; i<game_num_disks(g); i++) {
	d = game_disk(g, i);
	name = images_name(im, i);
	
	switch (result_image(res, i)) {
	case FS_UNKNOWN:
	case FS_BROKEN:
	    if (fix_options & FIX_PRINT)
		printf("%s: %s unknown image\n",
		       name,
		       ((fix_options & FIX_MOVE_UNKNOWN) ? "mv" : "delete"));
	    if (fix_options & FIX_DO) {
		if (fix_options & FIX_MOVE_UNKNOWN)
		    move_image_to_garbage(name);
		else
		    my_remove(name);
	    }
	    break;

	case FS_DUPLICATE:
	case FS_SUPERFLUOUS:
	    if (fix_options & FIX_PRINT)
		printf("%s: delete %s image\n",
		       name,
		       (result_image(res, i) == FS_SUPERFLUOUS
			? "unused" : "duplicate"));
	    if (fix_options & FIX_DO)
		my_remove(name);
	    remove_from_superfluous(name);
	    break;

	case FS_NEEDED:
	    if (fix_options & FIX_PRINT)
		printf("%s: save needed image\n", name);
	    save_needed_disk(name, (fix_options & FIX_DO));
	    break;

	case FS_MISSING:
	case FS_USED:
	case FS_PARTUSED:
	    break;
	}
    }
    
    for (i=0; i<game_num_disks(g); i++) {
	d = game_disk(g, i);
	md = result_disk(res, i);

	switch (match_disk_quality(md)) {
	case QU_COPIED:
	    if ((name=findfile(disk_name(d), TYPE_DISK)) != NULL) {
		/* XXX: move to garbage */
	    }
	    else
		fname = make_file_name(TYPE_DISK, 0, disk_name(d));
	    
	    if (fix_options & FIX_PRINT)
		printf("rename `%s' to `%s'\n",
		       match_disk_name(md), fname);
	    if (fix_options & FIX_DO)
		rename_or_move(match_disk_name(md), fname);
	    remove_from_superfluous(match_disk_name(md));
	    
	    free(fname);
	    break;

	case QU_HASHERR:
	    /* XXX: move to garbage */
	    break;

	default:
	    /* no fix necessary/possible */
	    break;
	}
    }

    return 0;
}



static int
fix_files(game_t *g, archive_t *a, result_t *res)
{
    struct zip_source *source;
    archive_t *afrom;
    struct zip *zfrom, *zto;
    match_t *m;
    rom_t *r;
    int i, idx, archive_changed;

    seterrinfo(NULL, archive_name(a));
    archive_ensure_zip(a, 1);
    zto = archive_zip(a);

    archive_changed = 0;

    for (i=0; i<game_num_files(g, file_type); i++) {
	m = result_rom(res, i);
	if (match_source_is_old(m))
	    afrom = NULL;
	else
	    afrom = match_archive(m);
	if (afrom) {
	    archive_ensure_zip(afrom, 0);
	    zfrom = archive_zip(afrom);
	}
	else
	    zfrom = NULL;
	r = game_file(g, file_type, i);
	seterrinfo(rom_name(r), archive_name(a));

	switch (match_quality(m)) {
	case QU_MISSING:
	    if (rom_size(r) == 0) {
		/* create missing empty file */
		if (fix_options & FIX_PRINT)
		    printf("%s: create empty file `%s'\n",
			   archive_name(a), rom_name(r));

		if (fix_options & FIX_DO) {
		    archive_changed = 1;
		    if ((source=zip_source_buffer(zto, NULL, 0, 0)) == NULL
			|| zip_add(zto, rom_name(r), source) < 0) {
			zip_source_free(source);
			myerror(ERRZIPFILE, "error creating empty file: %s",
				zip_strerror(zto));
		    }
		}
	    }
	    break;
	    
	case QU_HASHERR:
	    /* all is lost */
	    break;

	case QU_LONG:
	    if (fix_options & FIX_PRINT)
		printf("%s: extract (offset %" PRIdoff ", size %lu) from `%s'"
		       " to `%s'\n", archive_name(a),
		       PRIoff_cast match_offset(m), rom_size(r),
		       rom_name(archive_file(afrom, match_index(m))),
		       rom_name(r));
	    
	    if (fix_options & FIX_DO) {
		archive_changed = 1;
		if ((source=zip_source_zip(zto, zfrom, match_index(m),
					   ZIP_FL_UNCHANGED, match_offset(m),
					   rom_size(r))) == NULL
		    || zip_add(zto, rom_name(r), source) < 0) {
		    zip_source_free(source);
		    myerror(ERRZIPFILE, "error shrinking `%s': %s",
			    rom_name(archive_file(afrom, match_index(m))),
			    zip_strerror(zto));
		}
	    }
	    break;

	case QU_NAMEERR:
	    if (fix_options & FIX_PRINT)
		printf("%s: rename `%s' to `%s'\n", archive_name(a),
		       rom_name(archive_file(a, match_index(m))),
		       rom_name(r));
	    if (fix_options & FIX_DO) {
		archive_changed = 1;
		if (my_zip_rename(zto, match_index(m), rom_name(r)) == -1)
		    myerror(ERRZIPFILE, "error renaming `%s': %s",
			    rom_name(archive_file(a, match_index(m))),
			    zip_strerror(zto));
	    }
	    break;

	case QU_COPIED:
	    if (fix_options & FIX_PRINT)
		printf("%s: add `%s/%s' as `%s'\n",
		       archive_name(a), archive_name(afrom),
		       rom_name(archive_file(afrom, match_index(m))),
		       rom_name(r));
	    
	    if (fix_options & FIX_DO) {
		archive_changed = 1;
		/* make room for new file, if necessary */
		idx = archive_file_index_by_name(a, rom_name(r));
		if (idx >= 0) {
		    if (rom_status(archive_file(a, idx)) == STATUS_BADDUMP)
			zip_delete(zto, idx);
		    else
			idx = -1;
		}
		if ((source=zip_source_zip(zto, zfrom, match_index(m),
					   0, 0, -1)) == NULL
		    || zip_add(zto, rom_name(r), source) < 0) {
		    zip_source_free(source);
		    myerror(ERRZIPFILE, "error adding `%s' from `%s': %s",
			    rom_name(archive_file(afrom, match_index(m))),
			    archive_name(afrom), zip_strerror(zto));
		    if (idx >= 0)
			zip_unchange(zto, idx);
		}
		else {
		    if (match_where(m) == ROM_NEEDED)
			delete_list_add(needed_delete_list,
					archive_name(afrom), match_index(m));
		    else if (match_where(m) == ROM_SUPERFLUOUS)
			delete_list_add(superfluous_delete_list,
					archive_name(afrom), match_index(m));
		    else if (match_where(m) == ROM_EXTRA
			     && (fix_options & FIX_DELETE_EXTRA))
			delete_list_add(extra_delete_list,
					archive_name(afrom), match_index(m));
		}
	    }
	    break;

	case QU_INZIP:
	    /* ancestor must copy it first */
	    break;

	case QU_OK:
	    /* all is well */
	    break;

	case QU_NOHASH:
	    /* only used for disks */
	    break;

	case QU_OLD:
	    /* nothing to be done */
	    break;
	}
    }

    return archive_changed;
}
