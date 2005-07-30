/*
  $NiH: fix.c,v 1.2.2.1 2005/07/27 00:05:57 dillo Exp $

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
#include "file_by_hash.h"
#include "funcs.h"
#include "game.h"
#include "globals.h"
#include "match.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

extern char *prg;

static int fix_files(game_t *, archive_t *, match_array_t *);
static int fix_save_needed(archive_t *, int, int);
static struct zip *my_zip_open(const char *, int);

/* XXX: move to garbage.c */
static void close_garbage(void);
static int ensure_garbage_dir(void);
static int fix_add_garbage(archive_t *, int);
static char *mkgarbage_name(const char *);

static struct zip *zf_garbage;
static char *zf_garbage_name = NULL;



int
fix_game(game_t *g, archive_t *a, match_array_t *ma, match_disk_array_t *mda,
	 file_status_array_t *fsa)
{
    int i, islong, keep;

#if 0
    if (fix_options & (FIX_DO|FIX_PRINT))
    	fake_needed(me_a, me_fs);
#endif
    
    zf_garbage = NULL;

    if (fix_options & FIX_DO) {
	/* XXX: handle error */
	if (a == NULL)
	    a = archive_new(game_name(g), file_type, 1);
	else
	    archive_ensure_zip(a, 1);
    }

    for (i=0; i<archive_num_files(a); i++) {
	switch (file_status_array_get(fsa, i)) {
	case FS_UNKNOWN:
	case FS_PARTUSED:
	    islong = (file_status_array_get(fsa, i) == FS_PARTUSED);
	    keep = fix_options & (islong ? FIX_KEEP_UNKNOWN : FIX_KEEP_LONG);
	    if (fix_options & FIX_PRINT)
		printf("%s: %s %s file %s\n",
		       archive_name(a),
		       (keep ? "mv" : "rm"),
		       (islong ? "long" : "unknown"),
		       rom_name(archive_file(a, i)));
	    if (fix_options & FIX_DO) {
		if (keep)
		    keep = (fix_add_garbage(a, i) == -1);
		if (keep == 0)
		    zip_delete(archive_zip(a), i);
	    }
	    break;

	case FS_SUPERFLUOUS:
	    if (fix_options & FIX_PRINT)
		printf("%s: rm unused file %s\n",
		       archive_name(a),
		       rom_name(archive_file(a, i)));
	    if (fix_options & FIX_DO)
		zip_delete(archive_zip(a), i);
	    break;

	case FS_NEEDED:
	    if (fix_options & FIX_PRINT)
		printf("%s: save needed file %s\n",
		       archive_name(a),
		       rom_name(archive_file(a, i)));
	    if (fix_save_needed(a, i, fix_options & FIX_DO) != -1) {
		if (fix_options & FIX_DO)
		    zip_delete(archive_zip(a), i);
	    }
	    break;

	case FS_USED:
	    /* all is peachy */
	    break;
	}
    }

    if ((fix_options & (FIX_DO|FIX_PRINT)) == 0) {
	/* return early if no further messages or work requested */
	return 0;
    }

    fix_files(g, a, ma);

    close_garbage();

    return 0;
}



static int
fix_files(game_t *g, archive_t *a, match_array_t *ma)
{
    struct zip_source *source;
    archive_t *afrom;
    struct zip *zfrom, *zto;
    match_t *m;
    rom_t *r;
    int i;

    zto = archive_zip(a);

    for (i=0; i<game_num_files(g, file_type); i++) {
	m = match_array_get(ma, i);
	afrom = match_archive(m);
	if (afrom)
	    zfrom = archive_zip(afrom);
	else
	    zfrom = NULL;
	r = game_file(g, file_type, i);
	seterrinfo(rom_name(r), archive_name(a));

	switch (match_quality(m)) {
	case QU_MISSING:
	    /* all is lost */
	    break;

	case QU_LONG:
	    if (fix_options & FIX_PRINT)
		printf("%s: extract (offset %ld, size %lu) from `%s'"
		       " to `%s'\n", archive_name(a),
		       (long)match_offset(m), rom_size(r),
		       rom_name(archive_file(afrom, match_index(m))),
		       rom_name(r));
	    
	    if (fix_options & FIX_DO) {
		if ((source=zip_source_zip(zto, zfrom, match_index(m),
					   ZIP_FL_UNCHANGED, match_offset(m),
					   rom_size(r))) == NULL
		    || zip_add(zto, rom_name(r), source) < 0) {
		    zip_source_free(source);
		    myerror(ERRFILE, "error shrinking `%s': %s",
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
		if (zip_rename(zto, match_index(m), rom_name(r)) == -1)
		    myerror(ERRFILE, "error renaming `%s': %s",
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
		if ((source=zip_source_zip(zto, zfrom, match_index(m),
					   0, 0, -1)) == NULL
		    || zip_add(zto, rom_name(r), source) < 0) {
		    zip_source_free(source);
		    myerror(ERRFILE, "error adding `%s' from `%s': %s",
			    rom_name(archive_file(afrom, match_index(m))),
			    archive_name(afrom), zip_strerror(zto));
		}
	    }
	    break;

	case QU_INZIP:
	    /* ancestor must copy it first */
	    break;

	case QU_OK:
	    /* all is well */
	    break;

	case QU_NOCRC:
	case QU_CRCERR:
	    /* only used for disks */
	    break;
	}
    }

    return 0;
}



static struct zip *
my_zip_open(const char *name, int flags)
{
    struct zip *z;
    char errbuf[80];
    int err;

    z = zip_open(name, flags, &err);
    if (z == NULL)
	myerror(ERRDEF, "error creating zip archive `%s': %s", name,
		zip_error_to_str(errbuf, sizeof(errbuf), err, errno));

    return z;
}
    


static int
fix_save_needed(archive_t *a, int index, int copy)
{
    char *zip_name, *tmp;
    int ret, zip_index;
    struct zip *zto;
    struct zip_source *source;
    file_by_hash_t *fbh;

    zip_name = archive_name(a);
    zip_index = index;
    ret = 0;
    tmp = NULL;

    if (copy) {
	tmp = make_needed_name(archive_file(a, index));
	if ((zto=my_zip_open(tmp, ZIP_CREATE)) == NULL) {
	    ret = -1;
	}
	else if ((source=zip_source_zip(zto, archive_zip(a), index,
					0, 0, -1)) == NULL
		 || zip_add(zto, rom_name(archive_file(a, index)),
			    source) < 0) {
	    zip_source_free(source);
	    seterrinfo(tmp, rom_name(archive_file(a, index)));
	    myerror(ERRFILE, "error adding from `%s': %s",
		    archive_name(a), zip_strerror(zto));
	    ret = -1;
	}
	else {
	    zip_name = tmp;
	    zip_index = 0;
	}
    }

    fbh = file_by_hash_new(zip_name, zip_index);
    map_add(needed_map, file_by_hash_default_hashtype(TYPE_ROM),
	    rom_hashes(archive_file(a, index)), fbh);

    free(tmp);
    return ret;
}



static void
close_garbage(void)
{

    if (zf_garbage == NULL)
	return;

    if (zip_get_num_files(zf_garbage) > 0) {
	/*
	 * XXX: handle error, somehow, e.g.:
	 * . undo deletion of garbage files in a
	 * . if that fails, discard all changes and make big error message
	*/
	ensure_garbage_dir();
    }

    zip_close(zf_garbage);
}



static int
ensure_garbage_dir(void)
{
    char *s;
    struct stat st;

    s = strrchr(zf_garbage_name, '/');
    if (s == NULL) {
	/* internal error */
	myerror(ERRDEF, "internal error: no slash in "
		"zf_garbage_name `%s'", zf_garbage_name);
	return -1;
    }

    *s = 0;
    if (stat(zf_garbage_name, &st) < 0) {
	if (mkdir(zf_garbage_name, 0777) < 0) {
	    myerror(ERRDEF, "mkdir `%s' failed: %s",
		    zf_garbage_name, strerror(errno));
	    return -1;
	}
    }
    else if (!(st.st_mode & S_IFDIR)) {
	myerror(ERRDEF, "`%s' is not a directory", zf_garbage_name);
	return -1;
    }
    *s = '/';

    return 0;
}		    



static int
fix_add_garbage(archive_t *a, int idx)
{
    struct zip_source *source;
    int err;

    if (!(fix_options & FIX_DO))
	return 0;

    if (zf_garbage == NULL) {
	if (zf_garbage_name != NULL)
	    free(zf_garbage_name);
	zf_garbage_name = mkgarbage_name(archive_name(a));
	zf_garbage = zip_open(zf_garbage_name, ZIP_CREATE, &err);
	if (zf_garbage == NULL) {
	    char errbuf[80];

	    myerror(ERRDEF, "error creating garbage file `%s': %s",
		    zf_garbage_name,
		    zip_error_to_str(errbuf, sizeof(errbuf), err, errno));
	    return -1;
	}
    }

    if ((source=zip_source_zip(zf_garbage,
			       archive_zip(a), idx,
			       ZIP_FL_UNCHANGED, 0, -1)) == NULL
	|| zip_add(zf_garbage, rom_name(archive_file(a, idx)),
		   source) < 0) {
	zip_source_free(source);
	seterrinfo(archive_name(a), rom_name(archive_file(a, idx)));
	myerror(ERRFILE, "error moving to `%s': %s",
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
