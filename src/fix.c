/*
  fix.c -- fix ROM sets
  Copyright (C) 1999-2015 Dieter Baron and Thomas Klausner

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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zip.h>

#include "archive.h"
#include "error.h"
#include "file_location.h"
#include "funcs.h"
#include "game.h"
#include "garbage.h"
#include "globals.h"
#include "match.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

static int fix_disks(game_t *, images_t *, result_t *);
static int fix_files(game_t *, archive_t *, result_t *, garbage_t *);
static int fix_files_incomplete(game_t *g, archive_t *a, result_t *res, garbage_t *gb);


int
fix_game(game_t *g, archive_t *a, images_t *im, result_t *res) {
    int i;
    int ret;
    bool move;
    garbage_t *gb = NULL;

    if (fix_options & FIX_DO) {
	gb = garbage_new(a);

	if (archive_check(a) < 0) {
	    char *new_name;

	    /* opening the zip file failed, rename it and create new one */

	    if ((new_name = make_unique_name("broken", "%s", archive_name(a))) == NULL)
		return -1;

	    if (fix_options & FIX_PRINT)
		printf("%s: rename broken archive to '%s'\n", archive_name(a), new_name);
	    if (rename_or_move(archive_name(a), new_name) < 0) {
		free(new_name);
		return -1;
	    }
	    free(new_name);
	    if (archive_check(a) < 0)
		return -1;
	}

	if (extra_delete_list)
	    delete_list_mark(extra_delete_list);
	if (needed_delete_list)
	    delete_list_mark(needed_delete_list);
	if (superfluous_delete_list)
	    delete_list_mark(superfluous_delete_list);
    }

    for (i = 0; i < archive_num_files(a); i++) {
	switch (result_file(res, i)) {
	case FS_UNKNOWN:
	    if (fix_options & FIX_IGNORE_UNKNOWN)
		break;
	    move = (fix_options & FIX_MOVE_UNKNOWN);
	    if (fix_options & FIX_PRINT)
		printf("%s: %s unknown file '%s'\n", archive_name(a), (move ? "move" : "delete"), file_name(archive_file(a, i)));

	    if (fix_options & FIX_DO) {
		if (move)
		    garbage_add(gb, i, false); /* TODO: check return value */
		else
		    archive_file_delete(a, i); /* TODO: check return value */
	    }
	    break;

	case FS_DUPLICATE:
	    if (!(fix_options & FIX_DELETE_DUPLICATE))
		break;
	    /* fallthrough */
	case FS_SUPERFLUOUS:
	    if (fix_options & FIX_PRINT)
		printf("%s: delete %s file '%s'\n", archive_name(a), (result_file(res, i) == FS_SUPERFLUOUS ? "unused" : "duplicate"), file_name(archive_file(a, i)));

	    /* TODO: handle error (how?) */
	    archive_file_delete(a, i);
	    break;

	case FS_NEEDED:
	    /* TODO: handle error (how?) */
	    if (save_needed(a, i, game_name(g)) == 0)
		tree_recheck_games_needing(check_tree, file_size(archive_file(a, i)), file_hashes(archive_file(a, i)));
	    break;

	case FS_BROKEN:
	case FS_USED:
	case FS_MISSING:
	case FS_PARTUSED:
	    /* nothing to be done */
	    break;
	}
    }

    if (fix_options & FIX_DO) {
	if (garbage_commit(gb) < 0) {
	    /* TODO: error message? (or is message from archive_close enough?) */
	    /* TODO: handle error (how?) */
	    archive_rollback(a);
	    garbage_discard(gb);
	    myerror(ERRZIP, "committing garbage failed");
	    return -1;
	}
    }

    if ((fix_options & FIX_COMPLETE_ONLY) == 0 || res->game == GS_CORRECT || res->game == GS_FIXABLE) {
	ret = fix_files(g, a, res, gb);
    }
    else {
	ret = fix_files_incomplete(g, a, res, gb);
    }

    if (fix_options & FIX_DO) {
	if (garbage_close(gb) < 0) {
	    archive_rollback(a);
	    myerror(ERRZIP, "closing garbage failed");
	    return -1;
	}
    }

    if (archive_commit(a) < 0) {
	archive_rollback(a);
	if ((fix_options & FIX_DO) && extra_delete_list)
	    delete_list_rollback(extra_delete_list);
	if ((fix_options & FIX_DO) && needed_delete_list)
	    delete_list_rollback(needed_delete_list);
	if ((fix_options & FIX_DO) && superfluous_delete_list)
	    delete_list_rollback(superfluous_delete_list);
    }

    fix_disks(g, im, res);

    return ret;
}


static int
fix_disks(game_t *g, images_t *im, result_t *res) {
    int i;
    disk_t *d;
    match_disk_t *md;
    const char *name;
    char *fname = NULL;
    bool do_copy;
    int removed = 0;
    bool added = false;

    for (i = 0; i < images_length(im); i++) {
	name = images_name(im, i);

	switch (result_image(res, i)) {
	case FS_UNKNOWN:
	case FS_BROKEN:
	    if (fix_options & FIX_PRINT)
		printf("%s: %s unknown image\n", name, ((fix_options & FIX_MOVE_UNKNOWN) ? "move" : "delete"));
	    if (fix_options & FIX_DO) {
                if (fix_options & FIX_MOVE_UNKNOWN) {
		    move_image_to_garbage(name);
                }
                else {
		    my_remove(name);
                }
                removed += 1;
	    }
	    break;

	case FS_DUPLICATE:
	case FS_SUPERFLUOUS:
	    if (fix_options & FIX_PRINT)
		printf("%s: delete %s image\n", name, (result_image(res, i) == FS_SUPERFLUOUS ? "unused" : "duplicate"));
            if (fix_options & FIX_DO) {
		my_remove(name);
                removed += 1;
            }
	    remove_from_superfluous(name);
	    break;

	case FS_NEEDED:
	    if (fix_options & FIX_PRINT)
		printf("%s: save needed image\n", name);
	    save_needed_disk(name, (fix_options & FIX_DO));
            if (fix_options & FIX_DO) {
                removed += 1;
            }
	    break;

	case FS_MISSING:
	case FS_USED:
	case FS_PARTUSED:
	    break;
	}
    }

    for (i = 0; i < game_num_disks(g); i++) {
	d = game_disk(g, i);
	md = result_disk(res, i);

	switch (match_disk_quality(md)) {
	case QU_COPIED:
	    if ((name = findfile(disk_name(d), TYPE_DISK, game_name(g))) != NULL) {
		myerror(ERRDEF, "internal error: unknown disk '%s' exists, skipping", name);
		continue;
	    }

	    fname = make_file_name(TYPE_DISK, disk_name(d), game_name(g));

            switch (match_disk_where(md)) {
            case FILE_INGAME:
            case FILE_NEEDED:
            case FILE_SUPERFLUOUS:
                do_copy = 0;
                break;
                
            case FILE_INCO:
            case FILE_INGCO:
            case FILE_ROMSET:
                do_copy = 1;
                break;
                
            case FILE_EXTRA:
                do_copy = (fix_options & FIX_DELETE_EXTRA) == 0;
                break;
                
	    default:
                /* shouldn't happen */
                do_copy = 1;
                break;
            }
            
	    if (fix_options & FIX_PRINT)
		printf("%s '%s' to '%s'\n", do_copy ? "copy" : "rename", match_disk_name(md), fname);
	    if (fix_options & FIX_DO) {
                ensure_dir(fname, 1);
		if (do_copy) {
		    link_or_copy(match_disk_name(md), fname);
#if 0
		    /* delete_list_execute can't currently handle disks */
		    if (extra_delete_list)
		        delete_list_add(extra_delete_list, match_disk_name(md), 0);
#endif
		}
		else {
                    char *dir = mydirname(match_disk_name(md));
		    rename_or_move(match_disk_name(md), fname);
                    (void)rmdir(dir);
		    free(dir);
		    if (extra_list) {
			int idx;
			idx = parray_find_sorted(extra_list, match_disk_name(md), strcmp);
			if (idx >= 0)
			    parray_delete(extra_list, idx, free);
		    }
		}
                added = true;
	    }
	    remove_from_superfluous(match_disk_name(md));

	    free(fname);
	    break;

	case QU_HASHERR:
	    /* TODO: move to garbage */
	    break;

	default:
	    /* no fix necessary/possible */
	    break;
	}
    }
    
    if (!added && removed == images_length(im)) {
        char *dir;
        xasprintf(&dir, "%s/%s", get_directory(), game_name(g));
        (void)rmdir(dir);
        free(dir);
    }

    return 0;
}


static int
make_space(archive_t *a, const char *name, char **original_names, size_t num_names) {
    int idx = archive_file_index_by_name(a, name);

    if (idx < 0)
	return 0;

    if (idx < num_names && original_names[idx] == NULL)
	original_names[idx] = xstrdup(name);

    if (file_status(archive_file(a, idx)) == STATUS_BADDUMP) {
	if (fix_options & FIX_PRINT)
	    printf("%s: delete broken '%s'\n", archive_name(a), name);
	return archive_file_delete(a, idx);
    }

    return archive_file_rename_to_unique(a, idx);
}


#define REAL_NAME(aa, ii) ((aa) == a && (ii) < num_names && original_names[(ii)] ? original_names[(ii)] : file_name(archive_file((aa), (ii))))

static int
fix_files(game_t *g, archive_t *a, result_t *res, garbage_t *gb) {
    archive_t *afrom;
    match_t *m;
    file_t *r;
    int i;
    char **original_names;

    bool needs_recheck = false;

    seterrinfo(NULL, archive_name(a));

    size_t num_names = archive_num_files(a);
    original_names = xmalloc(sizeof(original_names[0]) * num_names);
    memset(original_names, 0, sizeof(original_names[0]) * num_names);

    for (i = 0; i < game_num_roms(g); i++) {
	m = result_rom(res, i);
	if (match_source_is_old(m))
	    afrom = NULL;
	else
	    afrom = match_archive(m);
	r = game_rom(g, i);
	seterrinfo(file_name(r), archive_name(a));

	switch (match_quality(m)) {
	case QU_MISSING:
	    if (file_size(r) == 0) {
		/* create missing empty file */
		if (fix_options & FIX_PRINT)
		    printf("%s: create empty file '%s'\n", archive_name(a), file_name(r));

		/* TODO: handle error (how?) */
		archive_file_add_empty(a, file_name(r));
	    }
	    break;

	case QU_HASHERR:
	    /* all is lost */
	    break;

	case QU_LONG:
	    if (a == afrom && (fix_options & FIX_MOVE_LONG) && file_where(archive_file(afrom, match_index(m))) != FILE_DELETED) {
		if (fix_options & FIX_PRINT)
		    printf("%s: move long file '%s'\n", archive_name(afrom), REAL_NAME(afrom, match_index(m)));
		if (garbage_add(gb, match_index(m), true) < 0)
		    break;
	    }

	    if (fix_options & FIX_PRINT)
		printf("%s: extract (offset %jd, size %" PRIu64 ") from '%s' to '%s'\n", archive_name(a), (intmax_t)match_offset(m), file_size(r), REAL_NAME(afrom, match_index(m)), file_name(r));

	    bool replacing_ourself = (a == afrom && match_index(m) == archive_file_index_by_name(afrom, file_name(r)));

	    if (make_space(a, file_name(r), original_names, num_names) < 0)
		break;
	    if (archive_file_copy_part(afrom, match_index(m), a, file_name(r), match_offset(m), file_size(r), r) < 0)
		break;

	    if (a == afrom && file_where(archive_file(afrom, match_index(m))) != FILE_DELETED) {
		if (!replacing_ourself && !(fix_options & FIX_MOVE_LONG) && (fix_options & FIX_PRINT))
		    printf("%s: delete long file '%s'\n", archive_name(afrom), file_name(r));
		archive_file_delete(afrom, match_index(m));
	    }
	    break;

	case QU_NAMEERR:
	    if (file_where(r) == FILE_INCO || file_where(r) == FILE_INGCO) {
		if (tree_recheck(check_tree, game_cloneof(g, file_where(r) - 1))) {
		    /* fall-through to rename in case save_needed fails */
		    if (save_needed(a, match_index(m), game_name(g)) == 0) {
			tree_recheck_games_needing(check_tree, file_size(r), file_hashes(r));
			break;
		    }
		}
	    }

	    if (fix_options & FIX_PRINT)
		printf("%s: rename '%s' to '%s'\n", archive_name(a), REAL_NAME(a, match_index(m)), file_name(r));

	    /* TODO: handle errors (how?) */
	    if (make_space(a, file_name(r), original_names, num_names) < 0)
		break;
	    archive_file_rename(a, match_index(m), file_name(r));

	    break;

	case QU_COPIED:
	    if (gb && afrom == gb->da) {
		/* we can't copy from our own garbage archive, since we're copying to it, and libzip doesn't support cross copying */

		/* TODO: handle error (how?) */
		if (save_needed(afrom, match_index(m), game_name(g)) == 0)
		    needs_recheck = true;

		break;
	    }
	    if (fix_options & FIX_PRINT)
		printf("%s: add '%s/%s' as '%s'\n", archive_name(a), archive_name(afrom), REAL_NAME(afrom, match_index(m)), file_name(r));

	    if (make_space(a, file_name(r), original_names, num_names) < 0) {
		/* TODO: if (idx >= 0) undo deletion of broken file */
		break;
	    }

	    if (archive_file_copy(afrom, match_index(m), a, file_name(r)) < 0) {
		myerror(ERRDEF, "copying '%s' from '%s' to '%s' failed, not deleting", file_name(r), archive_name(afrom), archive_name(a));
		/* TODO: if (idx >= 0) undo deletion of broken file */
	    }
	    else {
		delete_list_used(afrom, match_index(m));
	    }
	    break;

	case QU_INZIP:
	    /* TODO: save to needed */
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

    size_t ii;
    for (ii = 0; ii < num_names; ii++)
	free(original_names[ii]);
    free(original_names);

    return needs_recheck ? 1 : 0;
}


static int
fix_files_incomplete(game_t *g, archive_t *a, result_t *res, garbage_t *gb) {
    archive_t *afrom;
    match_t *m;
    file_t *r;
    int i;

    seterrinfo(NULL, archive_name(a));

    for (i = 0; i < game_num_roms(g); i++) {
	m = result_rom(res, i);
	if (match_source_is_old(m))
	    afrom = NULL;
	else
	    afrom = match_archive(m);
	r = game_rom(g, i);
	seterrinfo(file_name(r), archive_name(a));

	switch (match_quality(m)) {
	case QU_MISSING:
	case QU_OLD:
	    /* nothing to do */
	    break;

	case QU_NOHASH:
	    /* only used for disks */
	    break;
	    
	case QU_HASHERR:
	    /* all is lost */
	    break;

	case QU_LONG:
	    save_needed_part(afrom, match_index(m), game_name(g), match_offset(m), file_size(r), r); /* TODO: handle error */
	    break;

	case QU_COPIED:
	    switch(match_where(m)) {
	    case FILE_INGAME:
	    case FILE_SUPERFLUOUS:
	    case FILE_EXTRA:
		/* TODO: handle error (how?) */
		save_needed(afrom, match_index(m), game_name(g));
		archive_commit(afrom);
		break;
		
	    case FILE_INCO:
	    case FILE_INGCO:
	    case FILE_ROMSET:
	    case FILE_NEEDED:
	    case FILE_OLD:
            case FILE_NOWHERE:
            case FILE_ADDED:
            case FILE_DELETED:
		/* file is already where we will find it later */
		break;
	    }
	    break;

	case QU_NAMEERR:
	case QU_OK:
	case QU_INZIP:
	    save_needed(a, i, game_name(g)); /* TODO: handle error */
	    break;
	}
    }

    return 0;
}
