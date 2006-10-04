/*
  $NiH: check_util.c,v 1.4 2006/05/24 09:29:18 dillo Exp $

  util.c -- utility functions needed only by ckmame itself
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

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



#include <sys/stat.h>

#include "dir.h"
#include "funcs.h"
#include "globals.h"
#include "util.h"
#include "xmalloc.h"



#define MAXROMPATH 128
#define DEFAULT_ROMDIR "."



delete_list_t *extra_delete_list = NULL;
map_t *extra_disk_map = NULL;
map_t *extra_file_map = NULL;
parray_t *extra_list = NULL;
delete_list_t *needed_delete_list = NULL;
map_t *needed_disk_map = NULL;
map_t *needed_file_map = NULL;
delete_list_t *superfluous_delete_list = NULL;
char *needed_dir = "needed";	/* XXX: proper value, move elsewhere */
char *unknown_dir = "unknown";	/* XXX: proper value, move elsewhere */
char *rompath[MAXROMPATH] = { NULL };
static int rompath_init = 0;



static void enter_archive_in_map(map_t *, const archive_t *, where_t);
static int enter_dir_in_map_and_list(map_t *, map_t *, parray_t *,
				     const char *, int, where_t);
static void enter_disk_in_map(map_t *, const disk_t *, where_t);



void
ensure_extra_maps(int flags)
{
    int i;
    const char *file;
    archive_t *a;
    disk_t *d;
    name_type_t nt;

    if ((flags & (DO_MAP|DO_LIST)) == 0)
	return;

    if ((extra_file_map != NULL && !(flags & DO_LIST))
	|| (extra_list != NULL && !(flags & DO_MAP)))
	return;

    if (extra_file_map == NULL && (flags & DO_MAP)) {
	extra_disk_map = map_new();
	extra_file_map = map_new();
	extra_delete_list = delete_list_new();
	superfluous_delete_list = delete_list_new();
    }

    if (extra_list == NULL && (flags & DO_LIST))
	extra_list = parray_new();

    if (flags & DO_MAP) {
	for (i=0; i<parray_length(superfluous); i++) {
	    file = parray_get(superfluous, i);
	    switch ((nt=name_type(file))) {
	    case NAME_ZIP:
		if ((a=archive_new(file, 0)) != NULL) {
		    enter_archive_in_map(extra_file_map, a, ROM_SUPERFLUOUS);
		    archive_free(a);
		}
		break;
	    case NAME_CHD:
	    case NAME_NOEXT:
		if ((d=disk_new(file, nt==NAME_NOEXT)) != NULL) {
		    enter_disk_in_map(extra_disk_map, d, ROM_SUPERFLUOUS);
		    disk_free(d);
		}
		break;

	    default:
		/* ignore unknown files */
		break;
	    }
	}
    }

    for (i=0; i<parray_length(search_dirs); i++)
	enter_dir_in_map_and_list((flags & DO_MAP) ? extra_file_map : NULL,
				  (flags & DO_MAP) ? extra_disk_map : NULL,
				  (flags & DO_LIST) ? extra_list : NULL,
				  parray_get(search_dirs, i),
				  DIR_RECURSE, ROM_EXTRA);

    if (flags & DO_LIST)
	parray_sort(extra_list, strcmp);
}



void
ensure_needed_maps(void)
{
    if (needed_file_map != NULL)
	return;
    
    needed_disk_map = map_new();
    needed_file_map = map_new();
    needed_delete_list = delete_list_new();

    enter_dir_in_map_and_list(needed_file_map, needed_disk_map, NULL,
			      needed_dir, 0, ROM_NEEDED);
}



char *
findfile(const char *name, filetype_t what)
{
    int i;
    char *fn;
    struct stat st;

    if (what == TYPE_FULL_PATH) {
	if (stat(name, &st) == 0)
	    return xstrdup(name);
	else
	    return NULL;
    }

    init_rompath();

    for (i=0; rompath[i]; i++) {
	fn = make_file_name(what, i, name);
	if (stat(fn, &st) == 0)
	    return fn;
	if (what == TYPE_DISK) {
	    fn[strlen(fn)-4] = '\0';
	    if (stat(fn, &st) == 0)
		return fn;
	}
	free(fn);
    }
    
    return NULL;
}



void
init_rompath(void)
{
    int i, after;
    char *s, *e;

    if (rompath_init)
	return;

    /* skipping components placed via command line options */
    for (i=0; rompath[i]; i++)
	;

    if ((e = getenv("ROMPATH"))) {
	s = xstrdup(e);

	after = 0;
	if (s[0] == ':')
	    rompath[i++] = DEFAULT_ROMDIR;
	else if (s[strlen(s)-1] == ':')
	    after = 1;
	
	for (e=strtok(s, ":"); e; e=strtok(NULL, ":"))
	    rompath[i++] = e;

	if (after)
	    rompath[i++] = DEFAULT_ROMDIR;
    }
    else
	rompath[i++] = DEFAULT_ROMDIR;

    rompath[i] = NULL;

    rompath_init = 1;
}



char *
make_file_name(filetype_t ft, int idx, const char *name)
{
    char *fn, *dir, *ext;
    
    if (rompath_init == 0)
	init_rompath();

    if (ft == TYPE_SAMPLE)
	dir = "samples";
    else
	dir = "roms";

    if (ft == TYPE_DISK)
	ext = "chd";
    else
	ext = "zip";

    fn = xmalloc(strlen(rompath[idx])+strlen(dir)+strlen(name)+7);
    
    sprintf(fn, "%s/%s/%s.%s", rompath[idx], dir, name, ext);

    return fn;
}



static void
enter_archive_in_map(map_t *map, const archive_t *a, where_t where)
{
    int i;

    for (i=0; i<archive_num_files(a); i++)
	map_add(map, file_location_default_hashtype(TYPE_ROM),
		rom_hashes(archive_file(a, i)),
		file_location_ext_new(archive_name(a), i, where));
}



static int
enter_dir_in_map_and_list(map_t *zip_map, map_t *disk_map, parray_t *list,
			  const char *name, int flags, where_t where)
{
    dir_t *dir;
    dir_status_t ds;
    char b[8192];
    archive_t *a;
    disk_t *d;
    name_type_t nt;

    if ((dir=dir_open(name, flags)) == NULL)
	return -1;

    while ((ds=dir_next(dir, b, sizeof(b))) != DIR_EOD) {
	if (ds == DIR_ERROR) {
	    /* XXX: handle error */
	    continue;
	}
	switch ((nt=name_type(b))) {
	case NAME_ZIP:
	    if ((a=archive_new(b, 0)) != NULL) {
		if (zip_map)
		    enter_archive_in_map(zip_map, a, where);
		if (list)
		    parray_push(list, xstrdup(archive_name(a)));
		archive_free(a);
	    }
	    break;

	case NAME_CHD:
	case NAME_NOEXT:
	    if ((d=disk_new(b, nt==NAME_NOEXT)) != NULL) {
		if (disk_map)
		    enter_disk_in_map(disk_map, d, where);
		if (list)
		    parray_push(list, xstrdup(disk_name(d)));
		disk_free(d);
	    }

	case NAME_UNKNOWN:
	    /* ignore unknown files */
	    break;
	}
    }

    return dir_close(dir);
}    



static void
enter_disk_in_map(map_t *map, const disk_t *d, where_t where)
{
    map_add(map, file_location_default_hashtype(TYPE_DISK),
	    disk_hashes(d),
	    file_location_ext_new(disk_name(d), 0, where));
}
