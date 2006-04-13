/*
  $NiH: fix.c,v 1.13 2006/04/13 22:38:39 dillo Exp $

  fix.c -- fix ROM sets
  Copyright (C) 1999, 2004, 2005 Dieter Baron and Thomas Klausner

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
#include "globals.h"
#include "match.h"
#include "pri.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

#define MARK_DELETED(x, i)	(*(int *)array_get((x), (i)) = 1)
#define IS_DELETED(x, i)	(*(int *)array_get((x), (i)) == 1)

static int fix_disks(game_t *, result_t *res);
static int fix_files(game_t *, archive_t *, result_t *);
static int fix_save_needed(archive_t *, int, int);
static int fix_save_needed_disk(const char *, int);
static void set_zero(int *);

/* XXX: move to garbage.c */
static int close_garbage(void);
static int fix_add_garbage(archive_t *, int);
static char *mkgarbage_name(const char *);
static int myremove(const char *);

static struct zip *zf_garbage;
static char *zf_garbage_name = NULL;



int
fix_game(game_t *g, archive_t *a, result_t *res)
{
    int i, islong, keep, archive_changed;
    array_t *deleted;

    zf_garbage = NULL;

    archive_changed = 0;

    if (fix_options & FIX_DO) {
	if (archive_ensure_zip(a, 1) < 0) {
	    char *new_name;

	    /* opening the zip file failed, rename it and create new one */

	    if ((new_name=make_unique_name("broken", "%s",
					   archive_name(a))) == NULL)
		return -1;

	    if (rename_or_move(archive_name(a), new_name) < 0) {
		free(new_name);
		return -1;
	    }
	    free(new_name);
	    if (archive_ensure_zip(a, 1) < 0)
		return -1;
	}
	deleted = array_new_length(sizeof(int), archive_num_files(a),
				   set_zero);
	if (needed_delete_list)
	    delete_list_mark(needed_delete_list);
    }

    for (i=0; i<archive_num_files(a); i++) {
	switch (result_file(res, i)) {
	case FS_UNKNOWN:
	case FS_PARTUSED:
	    islong = (result_file(res, i) == FS_PARTUSED);
	    keep = fix_options & (islong ? FIX_KEEP_UNKNOWN : FIX_KEEP_LONG);
	    if (fix_options & FIX_PRINT)
		printf("%s: %s %s file `%s'\n",
		       archive_name(a),
		       (keep ? "mv" : "delete"),
		       (islong ? "long" : "unknown"),
		       rom_name(archive_file(a, i)));
	    if (fix_options & FIX_DO) {
		if (keep)
		    keep = (fix_add_garbage(a, i) == -1);
		if (keep == 0) {
		    archive_changed = 1;
		    MARK_DELETED(deleted, i);
		    zip_delete(archive_zip(a), i);
		}
	    }
	    break;

	case FS_SUPERFLUOUS:
	    if (fix_options & FIX_PRINT)
		printf("%s: delete unused file `%s'\n",
		       archive_name(a),
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
	    if (fix_save_needed(a, i, fix_options & FIX_DO) != -1) {
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
	if (close_garbage() < 0) {
	    /* undelete files we tried to move to garbage */
	    for (i=0; i<archive_num_files(a); i++) {
		if (IS_DELETED(deleted, i)) {
		    if (zip_unchange(archive_zip(a), i) < 0) {
			/* XXX: cannot undelete */
		    }
		}
	    }
	}
	array_free(deleted, NULL);
    }
    
    if ((fix_options & (FIX_DO|FIX_PRINT)) == 0) {
	/* return early if no further messages or work requested */
	return 0;
    }

    archive_changed |= fix_files(g, a, res);

    if (archive_close_zip(a) < 0) {
	archive_changed = 0;
	if ((fix_options & FIX_DO) && needed_delete_list)
	    delete_list_rollback(needed_delete_list);
    }

    fix_disks(g, res);

    return archive_changed;
}



static int
fix_disks(game_t *g, result_t *res)
{
    int i;
    disk_t *d;
    match_disk_t *md;
    char *name, *to_name;
    
    for (i=0; i<game_num_disks(g); i++) {
	d = game_disk(g, i);
	name = result_disk_name(res, i);
	
	switch (result_disk_file(res, i)) {
	case FS_UNKNOWN:
	case FS_BROKEN:
	    if (fix_options & FIX_PRINT)
		printf("%s: %s unknown image\n",
		       name,
		       ((fix_options & FIX_KEEP_UNKNOWN) ? "mv" : "delete"));
	    if (fix_options & FIX_DO) {
		if (fix_options & FIX_KEEP_UNKNOWN) {
		    to_name = mkgarbage_name(name);
		    ensure_dir(to_name, 1);
		    rename_or_move(name, to_name);
		    free(to_name);
		}
		else
		    myremove(name);
	    }
	    break;

	case FS_SUPERFLUOUS:
	    if (fix_options & FIX_PRINT)
		printf("%s: delete unused image\n", name);
	    if (fix_options & FIX_DO)
		myremove(name);
	    break;

	case FS_NEEDED:
	    if (fix_options & FIX_PRINT)
		printf("%s: save needed image\n", name);
	    fix_save_needed_disk(name, (fix_options & FIX_DO));
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
		name = make_file_name(TYPE_DISK, 0, disk_name(d));
	    
	    if (fix_options & FIX_PRINT)
		printf("rename `%s' to `%s'\n",
		       match_disk_name(md), name);
	    if (fix_options & FIX_DO)
		rename_or_move(match_disk_name(md), name);
	    
	    free(name);
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
	afrom = match_archive(m);
	if (afrom)
	    zfrom = archive_zip(afrom);
	else
	    zfrom = NULL;
	r = game_file(g, file_type, i);
	seterrinfo(rom_name(r), archive_name(a));

	switch (match_quality(m)) {
	case QU_MISSING:
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
		    if (match_where(m) == ROM_NEEDED
			|| match_where(m) == ROM_SUPERFLUOUS
			|| (match_where(m) == ROM_EXTRA
			    && (fix_options & FIX_DELETE_EXTRA)))
			delete_list_add(needed_delete_list,
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
	}
    }

    return archive_changed;
}



static int
fix_save_needed(archive_t *a, int index, int copy)
{
    char *zip_name, *tmp;
    int ret, zip_index;
    struct zip *zto;
    struct zip_source *source;
    file_location_ext_t *fbh;

    zip_name = archive_name(a);
    zip_index = index;
    ret = 0;
    tmp = NULL;

    if (copy) {
	tmp = make_needed_name(archive_file(a, index));
	if (tmp == NULL) {
	    myerror(ERRDEF, "cannot create needed file name");
	    ret = -1;
	}
	else if (ensure_dir(tmp, 1) < 0)
	    ret = -1;
	else if ((zto=my_zip_open(tmp, ZIP_CREATE)) == NULL)
	    ret = -1;
	else if ((source=zip_source_zip(zto, archive_zip(a), index,
					0, 0, -1)) == NULL
		 || zip_add(zto, rom_name(archive_file(a, index)),
			    source) < 0) {
	    zip_source_free(source);
	    seterrinfo(rom_name(archive_file(a, index)), tmp);
	    myerror(ERRZIPFILE, "error adding from `%s': %s",
		    archive_name(a), zip_strerror(zto));
	    zip_close(zto);
	    ret = -1;
	}
	else if (zip_close(zto) < 0) {
	    seterrinfo(NULL, tmp);
	    myerror(ERRZIP, "error closing: %s", zip_strerror(zto));
	    zip_unchange_all(zto);
	    zip_close(zto);
	    ret = -1;
	}
	else {
	    zip_name = tmp;
	    zip_index = 0;
	}
    }

    fbh = file_location_ext_new(zip_name, zip_index, ROM_NEEDED);
    map_add(needed_file_map, file_location_default_hashtype(TYPE_ROM),
	    rom_hashes(archive_file(a, index)), fbh);

    free(tmp);
    return ret;
}



static int
fix_save_needed_disk(const char *fname, int move)
{
    char *tmp;
    const char *name;
    int ret;
    disk_t *d;

    if ((d=disk_get_info(fname, 0)) == NULL)
	return -1;

    ret = 0;
    tmp = NULL;
    name = fname;

    if (move) {
	tmp = make_needed_name_disk(d);
	if (tmp == NULL) {
	    myerror(ERRDEF, "cannot create needed file name");
	    ret = -1;
	}
	else if (ensure_dir(tmp, 1) < 0)
	    ret = -1;
	else if (rename_or_move(fname, tmp) != 0)
	    ret = -1;
	else
	    name = tmp;
    }

    ensure_needed_maps();
    map_add(needed_disk_map, file_location_default_hashtype(TYPE_DISK),
	    disk_hashes(d), file_location_ext_new(name, 0, ROM_NEEDED));

    disk_free(d);
    free(tmp);
    return ret;
}



static int
close_garbage(void)
{
    if (zf_garbage == NULL)
	return 0;

    if (zip_get_num_files(zf_garbage) > 0) {
	if (ensure_dir(zf_garbage_name, 1) < 0) {
	    zip_unchange_all(zf_garbage);
	    zip_close(zf_garbage);
	    return -1;
	}
    }

    if (zip_close(zf_garbage) < 0) {
	zip_unchange_all(zf_garbage);
	zip_close(zf_garbage);
	return -1;
    }

    return 0;
}



static int
fix_add_garbage(archive_t *a, int idx)
{
    struct zip_source *source;

    if (!(fix_options & FIX_DO))
	return 0;

    if (zf_garbage == NULL) {
	if (zf_garbage_name != NULL)
	    free(zf_garbage_name);
	zf_garbage_name = mkgarbage_name(archive_name(a));
	if ((zf_garbage=my_zip_open(zf_garbage_name, ZIP_CREATE)) == NULL)
	    return -1;
    }

    if ((source=zip_source_zip(zf_garbage, archive_zip(a), idx,
			       ZIP_FL_UNCHANGED, 0, -1)) == NULL
	|| zip_add(zf_garbage, rom_name(archive_file(a, idx)), source) < 0) {
	zip_source_free(source);
	seterrinfo(archive_name(a), rom_name(archive_file(a, idx)));
	myerror(ERRZIPFILE, "error moving to `%s': %s",
		zf_garbage_name, zip_strerror(zf_garbage));
	return -1;
    }

    return 0;
}



static char *
mkgarbage_name(const char *name)
{
    const char *s;
    char *t;

    if ((s=strrchr(name, '/')) == NULL)
	s = name;
    else
	s++;

    t = (char *)xmalloc(strlen(name)+strlen("garbage/")+1);

    sprintf(t, "%.*sgarbage/%s", (int)(s-name), name, s);

    return t;
}



static void
set_zero(int *ip)
{
    *ip = 0;
}



static int
myremove(const char *name)
{
    if (remove(name) != 0) {
	seterrinfo(name, NULL);
	myerror(ERRFILESTR, "cannot remove");
	return -1;
    }

    return 0;
}
