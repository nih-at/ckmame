/*
  $NiH: check_util.c,v 1.5 2006/10/04 17:36:43 dillo Exp $

  check_util.c -- utility functions needed only by ckmame itself
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

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



#include <sys/stat.h>

#include "dir.h"
#include "funcs.h"
#include "globals.h"
#include "memdb.h"
#include "sq_util.h"
#include "util.h"
#include "xmalloc.h"



#define MAXROMPATH 128
#define DEFAULT_ROMDIR "."

#define EXTRA_MAPS		0x1
#define NEEDED_MAPS		0x2

#define INSERT_FILE	\
    "insert into file (game_id, file_type, file_idx, location," \
    " size, crc, md5, sha1) values (?, ?, ?, ?, ?, ?, ?, ?)"



int maps_done = 0;
delete_list_t *extra_delete_list = NULL;
parray_t *extra_list = NULL;
delete_list_t *needed_delete_list = NULL;
delete_list_t *superfluous_delete_list = NULL;
char *needed_dir = "needed";	/* XXX: proper value, move elsewhere */
char *unknown_dir = "unknown";	/* XXX: proper value, move elsewhere */
char *rompath[MAXROMPATH] = { NULL };
static int rompath_init = 0;



static int enter_dir_in_map_and_list(int, parray_t *, const char *,
				     int, where_t);



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

    if (((maps_done & EXTRA_MAPS) && !(flags & DO_LIST))
	|| (extra_list != NULL && !(flags & DO_MAP)))
	return;

    if (!(maps_done & EXTRA_MAPS) && (flags & DO_MAP)) {
	maps_done |= EXTRA_MAPS;
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
		    enter_archive_in_map(a, FILE_SUPERFLUOUS);
		    archive_free(a);
		}
		break;
	    case NAME_CHD:
	    case NAME_NOEXT:
		if ((d=disk_new(file, (nt==NAME_NOEXT
				       ? DISK_FL_QUIET : 0))) != NULL) {
		    enter_disk_in_map(d, FILE_SUPERFLUOUS);
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
	enter_dir_in_map_and_list(flags, extra_list,
				  parray_get(search_dirs, i),
				  DIR_RECURSE, FILE_EXTRA);

    if (flags & DO_LIST)
	parray_sort(extra_list, strcmp);
}



void
ensure_needed_maps(void)
{
    if (maps_done & NEEDED_MAPS)
	return;
    
    maps_done |= NEEDED_MAPS;
    needed_delete_list = delete_list_new();

    enter_dir_in_map_and_list(DO_MAP,NULL, needed_dir, 0, FILE_NEEDED);
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



int
enter_archive_in_map(const archive_t *a, where_t where)
{
    sqlite3_stmt *stmt;
    int i;
    file_t *r;

    if (sqlite3_prepare_v2(memdb, INSERT_FILE, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    if (sqlite3_bind_int(stmt, 1, archive_id(a)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, TYPE_ROM) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return -1;
    }

    for (i=0; i<archive_num_files(a); i++) {
	r = archive_file(a, i);

	if (file_status(r) != STATUS_OK)
	    continue;

	if (sqlite3_bind_int(stmt, 3, i) != SQLITE_OK
	    || sqlite3_bind_int(stmt, 4, where) != SQLITE_OK
	    || sq3_set_int64_default(stmt, 5, file_size(r),
				     SIZE_UNKNOWN) != SQLITE_OK
	    || sq3_set_hashes(stmt, 6, file_hashes(r), 1) != SQLITE_OK
	    || sqlite3_step(stmt) != SQLITE_DONE
	    || sqlite3_reset(stmt) != SQLITE_OK) {
	    sqlite3_finalize(stmt);
	    return -1;
	}
    }

    sqlite3_finalize(stmt);

    return 0;
}



static int
enter_dir_in_map_and_list(int flags, parray_t *list, const char *name,
			  int dir_flags, where_t where)
{
    dir_t *dir;
    dir_status_t ds;
    char b[8192];
    archive_t *a;
    disk_t *d;
    name_type_t nt;

    if ((dir=dir_open(name, dir_flags)) == NULL)
	return -1;

    while ((ds=dir_next(dir, b, sizeof(b))) != DIR_EOD) {
	if (ds == DIR_ERROR) {
	    /* XXX: handle error */
	    continue;
	}
	switch ((nt=name_type(b))) {
	case NAME_ZIP:
	    if ((a=archive_new(b, 0)) != NULL) {
		if (flags & DO_MAP)
		    enter_archive_in_map(a, where);
		if ((flags & DO_LIST) && list)
		    parray_push(list, xstrdup(archive_name(a)));
		archive_free(a);
	    }
	    break;

	case NAME_CHD:
	case NAME_NOEXT:
	    if ((d=disk_new(b, (nt==NAME_NOEXT
				? DISK_FL_QUIET : 0))) != NULL) {
		if (flags & DO_MAP)
		    enter_disk_in_map(d, where);
		if ((flags & DO_LIST) && list)
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



int
enter_disk_in_map(const disk_t *d, where_t where)
{
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(memdb, INSERT_FILE, -1, &stmt, NULL) != SQLITE_OK)
	return -1;

    if (sqlite3_bind_int(stmt, 1, disk_id(d)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, TYPE_DISK) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 3, 0) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 4, where) != SQLITE_OK
	|| sqlite3_bind_null(stmt, 5) != SQLITE_OK
	|| sq3_set_hashes(stmt, 6, disk_hashes(d), 1) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_DONE) {
	sqlite3_finalize(stmt);
	return -1;
    }

    sqlite3_finalize(stmt);

    return 0;
}



int
enter_file_in_map(const archive_t *a, int idx, where_t where)
{
    sqlite3_stmt *stmt;
    file_t *r;

    r = archive_file(a, idx);

    if (file_status(r) != STATUS_OK)
	return 0;

    if (sqlite3_prepare_v2(memdb, INSERT_FILE, -1, &stmt, NULL) != SQLITE_OK)
	return -1;
    
    if (sqlite3_bind_int(stmt, 1, archive_id(a)) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 2, TYPE_ROM) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 3, idx) != SQLITE_OK
	|| sqlite3_bind_int(stmt, 4, where) != SQLITE_OK
	|| sq3_set_int64_default(stmt, 5, file_size(r),
				 SIZE_UNKNOWN) != SQLITE_OK
	|| sq3_set_hashes(stmt, 6, file_hashes(r), 1) != SQLITE_OK
	|| sqlite3_step(stmt) != SQLITE_DONE) {
	sqlite3_finalize(stmt);
	return -1;
    }

    sqlite3_finalize(stmt);

    return 0;
}
