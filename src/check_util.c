/*
  check_util.c -- utility functions needed only by ckmame itself
  Copyright (C) 1999-2013 Dieter Baron and Thomas Klausner

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


#define EXTRA_MAPS		0x1
#define NEEDED_MAPS		0x2


int maps_done = 0;


static int enter_dir_in_map_and_list(int, parray_t *, const char *, int, where_t);


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
		if ((a=archive_new(file, TYPE_ROM, FILE_SUPERFLUOUS, 0)) != NULL) {
		    archive_free(a);
		}
		break;
	    case NAME_CHD:
	    case NAME_NOEXT:
		if ((d=disk_new(file, (nt==NAME_NOEXT ? DISK_FL_QUIET : 0))) != NULL) {
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
	enter_dir_in_map_and_list(flags, extra_list, parray_get(search_dirs, i), DIR_RECURSE, FILE_EXTRA);

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
    char *fn;
    struct stat st;

    if (what == TYPE_FULL_PATH) {
	if (stat(name, &st) == 0)
	    return xstrdup(name);
	else
	    return NULL;
    }

    fn = make_file_name(what, name);
    if (stat(fn, &st) == 0)
	return fn;
    if (what == TYPE_DISK) {
	fn[strlen(fn)-4] = '\0';
	if (stat(fn, &st) == 0)
	return fn;
    }
    free(fn);
    
    return NULL;
}





char *
make_file_name(filetype_t ft, const char *name)
{
    char *fn;
    const char *dir, *ext;
    
    dir = get_directory(ft);

    if (ft == TYPE_DISK)
	ext = ".chd";
    else
	ext = roms_unzipped ? "" : ".zip";

    fn = xmalloc(strlen(dir)+strlen(name)+7);
    
    sprintf(fn, "%s/%s%s", dir, name, ext);

    return fn;
}



static int
enter_dir_in_map_and_list(int flags, parray_t *list, const char *name, int dir_flags, where_t where)
{
    dir_t *dir;
    dir_status_t ds;
    char b[8192];
    archive_t *a;
    disk_t *d;
    name_type_t nt;

    if ((dir=dir_open(name, dir_flags)) == NULL)
	return -1;

    if (roms_unzipped) {
	if ((a=archive_new(name, TYPE_ROM, where, ARCHIVE_FL_DELAY_READINFO)) == NULL) {
	    /* TODO: handle error */
	}
    }

    while ((ds=dir_next(dir, b, sizeof(b))) != DIR_EOD) {
	int handled = 0;
	if (ds == DIR_ERROR) {
	    /* TODO: handle error */
	    continue;
	}
	switch ((nt=name_type(b))) {
	case NAME_ZIP:
	    if (roms_unzipped)
		break;
	    if ((a=archive_new(b, TYPE_ROM, where, 0)) != NULL) {
		if ((flags & DO_LIST) && list)
		    parray_push(list, xstrdup(archive_name(a)));
		archive_free(a);
	    }
	    break;

	case NAME_CHD:
	case NAME_NOEXT:
	    if ((d=disk_new(b, (nt==NAME_NOEXT ? DISK_FL_QUIET : 0))) != NULL) {
		if (flags & DO_MAP)
		    enter_disk_in_map(d, where);
		if ((flags & DO_LIST) && list)
		    parray_push(list, xstrdup(disk_name(d)));
		disk_free(d);
		handled = 1;
	    }

	case NAME_UNKNOWN:
	    /* ignore unknown files */
	    break;
	}
	if (roms_unzipped && !handled) {
	    archive_dir_add_file(a, b+strlen(name)+1, NULL, NULL); /* TODO: handle error */
	}
    }

    if (roms_unzipped) {
	if ((flags & DO_LIST) && list)
	    parray_push(list, xstrdup(name));
	archive_free(a);
    }

    return dir_close(dir);
}



int
enter_disk_in_map(const disk_t *d, where_t where)
{
    sqlite3_stmt *stmt;

    if ((stmt = dbh_get_statement(memdb, DBH_STMT_MEM_INSERT_FILE)) == NULL)
	return -1;

    if (sqlite3_bind_int(stmt, 1, disk_id(d)) != SQLITE_OK
        || sqlite3_bind_int(stmt, 2, TYPE_DISK) != SQLITE_OK
        || sqlite3_bind_int(stmt, 3, 0) != SQLITE_OK
        || sqlite3_bind_int(stmt, 4, FILE_SH_FULL) != SQLITE_OK
        || sqlite3_bind_int(stmt, 5, where) != SQLITE_OK
        || sqlite3_bind_null(stmt, 6) != SQLITE_OK
        || sq3_set_hashes(stmt, 7, disk_hashes(d), 1) != SQLITE_OK
        || sqlite3_step(stmt) != SQLITE_DONE)
	return -1;

    return 0;
}
