/*
  check_util.c -- utility functions needed only by ckmame itself
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

#include <algorithm>

#include <sys/param.h>
#include <sys/stat.h>

#include "dbh_cache.h"
#include "dir_old.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "memdb.h"
#include "sq_util.h"
#include "util.h"
#include "xmalloc.h"

#define EXTRA_MAPS 0x1
#define NEEDED_MAPS 0x2

int maps_done = 0;

static int enter_dir_in_map_and_list(int, parray_t *, const char *, int, where_t);
static int enter_dir_in_map_and_list_unzipped(int, parray_t *, const char *, int, where_t);
static int enter_dir_in_map_and_list_zipped(int, parray_t *, const char *, int, where_t);
static int enter_file_in_map_and_list(int flags, parray_t *list, const char *name, where_t where);


void
ensure_extra_maps(int flags) {
    int i;
    const char *file;
    name_type_t nt;

    if ((flags & (DO_MAP | DO_LIST)) == 0) {
	return;
    }

    if (((maps_done & EXTRA_MAPS) && !(flags & DO_LIST)) || (extra_list != NULL && !(flags & DO_MAP))) {
	return;
    }

    if (!(maps_done & EXTRA_MAPS) && (flags & DO_MAP)) {
	maps_done |= EXTRA_MAPS;
	extra_delete_list = delete_list_new();
	superfluous_delete_list = delete_list_new();
    }

    if (extra_list == NULL && (flags & DO_LIST))
	extra_list = parray_new();

    if (flags & DO_MAP) {
	memdb_ensure();
	for (i = 0; i < parray_length(superfluous); i++) {
	    file = static_cast<const char *>(parray_get(superfluous, i));
	    switch ((nt = name_type(file))) {
		case NAME_ZIP: {
		    auto a = Archive::open(file, TYPE_ROM, FILE_SUPERFLUOUS, 0);
		    if (a) {
			a->close();
		    }
		    break;
		}

		case NAME_CHD: {
		    auto disk = Disk::from_file(file, 0);
		    if (disk) {
			enter_disk_in_map(disk.get(), FILE_SUPERFLUOUS);
		    }
		    break;
		}

	    default:
		/* ignore unknown files */
		break;
	    }
	}

	if (roms_unzipped) {
	    ArchivePtr a = Archive::open_toplevel(get_directory(), TYPE_ROM, FILE_SUPERFLUOUS, 0);
	    if (a) {
		a->close();
	    }
	}
    }

    for (i = 0; i < parray_length(search_dirs); i++)
	enter_dir_in_map_and_list(flags, extra_list, static_cast<const char *>(parray_get(search_dirs, i)), DIR_RECURSE, FILE_EXTRA);

    if (flags & DO_LIST)
	parray_sort(extra_list, reinterpret_cast<int (*)(const void *, const void *)>(strcmp));
}


void
ensure_needed_maps(void) {
    if (maps_done & NEEDED_MAPS)
	return;

    maps_done |= NEEDED_MAPS;
    needed_delete_list = delete_list_new();

    enter_dir_in_map_and_list(DO_MAP, NULL, needed_dir, DIR_RECURSE, FILE_NEEDED);
}


char *
findfile(const char *name, filetype_t what, const char *game_name) {
    char *fn;
    struct stat st;

    if (what == TYPE_FULL_PATH) {
	if (stat(name, &st) == 0)
	    return xstrdup(name);
	else
	    return NULL;
    }

    fn = make_file_name(what, name, game_name);
    if (stat(fn, &st) == 0)
	return fn;
    if (what == TYPE_DISK) {
	fn[strlen(fn) - 4] = '\0';
	if (stat(fn, &st) == 0)
	    return fn;
    }
    free(fn);

    return NULL;
}


char *
make_file_name(filetype_t ft, const char *name, const char *game_name) {
    char *fn;
    const char *dir, *ext;

    dir = get_directory();

    if (ft == TYPE_DISK) {
	ext = ".chd";
    }
    else {
	ext = roms_unzipped ? "" : ".zip";
	game_name = NULL;
    }

    xasprintf(&fn, "%s/%s%s%s%s", dir, game_name ? game_name : "", game_name ? "/" : "", name, ext);

    return fn;
}


static int
enter_dir_in_map_and_list(int flags, parray_t *list, const char *directory_name, int dir_flags, where_t where) {
    parray_t *our_list;

    if (list == NULL) {
	our_list = parray_new();
    }
    else {
	our_list = list;
    }

    int ret;
    if (roms_unzipped) {
	ret = enter_dir_in_map_and_list_unzipped(flags | DO_LIST, our_list, directory_name, dir_flags, where);
    }
    else {
	ret = enter_dir_in_map_and_list_zipped(flags | DO_LIST, our_list, directory_name, dir_flags, where);
    }

    if (ret == 0) {
	/* clean up cache db: remove archives no longer in file system */
	dbh_t *dbh = dbh_cache_get_db_for_archive(directory_name);
	if (dbh) {
	    auto list_db = dbh_cache_list_archives(dbh);
	    if (!list_db.empty()) {
		std::sort(list_db.begin(), list_db.end());

		for (auto entry_name : list_db) {
		    std::string name = directory_name;
		    if (entry_name != ".") {
			name += '/' + entry_name;
			if (!roms_unzipped) {
			    name += ".zip";
			}
		    }
		    if (parray_find(our_list, name.c_str(), reinterpret_cast<int (*)(const void *, const void *)>(strcmp)) == -1) {
			dbh_cache_delete_by_name(dbh, name.c_str());
		    }
		}
	    }
	}
    }

    if (list == NULL) {
	parray_free(our_list, free);
    }

    return ret;
}

static int
enter_dir_in_map_and_list_unzipped(int flags, parray_t *list, const char *directory_name, int dir_flags, where_t where) {
    dir_t *dir;
    dir_status_t ds;
    char name[8192];

    if ((dir = dir_open(directory_name, dir_flags & ~DIR_RECURSE)) == NULL) {
	return -1;
    }

    while ((ds = dir_next(dir, name, sizeof(name))) != DIR_EOD) {
	struct stat sb;
	if (strcmp(mybasename(name), DBH_CACHE_DB_NAME) == 0) {
	    continue;
	}
	if (stat(name, &sb) < 0) {
	    continue;
	}
	if (S_ISDIR(sb.st_mode)) {
	    auto a = Archive::open(name, TYPE_ROM, where, 0);
	    if (a) {
		if ((flags & DO_LIST) && list) {
		    parray_push(list, xstrdup(name));
		}
		a->close();
	    }
	}
    }

    auto a = Archive::open_toplevel(directory_name, TYPE_ROM, where, 0);
    if (a) {
	if ((flags & DO_LIST) && list) {
	    parray_push(list, xstrdup(a->name.c_str()));
	}
	a->close();
    }

    return dir_close(dir);
}


static int
enter_dir_in_map_and_list_zipped(int flags, parray_t *list, const char *dir_name, int dir_flags, where_t where) {
    dir_t *dir;
    dir_status_t ds;
    char b[8192];

    if ((dir = dir_open(dir_name, dir_flags)) == NULL) {
	return -1;
    }

    while ((ds = dir_next(dir, b, sizeof(b))) != DIR_EOD) {
	if (ds == DIR_ERROR) {
	    /* TODO: handle error */
	    continue;
	}

	enter_file_in_map_and_list(flags, list, b, where);
	/* TODO: handle error */
    }

    return dir_close(dir);
}


int
enter_disk_in_map(const Disk *d, where_t where) {
    sqlite3_stmt *stmt;

    if ((stmt = dbh_get_statement(memdb, DBH_STMT_MEM_INSERT_FILE)) == NULL)
	return -1;

    if (sqlite3_bind_int64(stmt, 1, disk_id(d)) != SQLITE_OK || sqlite3_bind_int(stmt, 2, TYPE_DISK) != SQLITE_OK || sqlite3_bind_int(stmt, 3, 0) != SQLITE_OK || sqlite3_bind_int(stmt, 4, 0) != SQLITE_OK || sqlite3_bind_int(stmt, 5, where) != SQLITE_OK || sqlite3_bind_null(stmt, 6) != SQLITE_OK || sq3_set_hashes(stmt, 7, disk_hashes(d), 1) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE)
	return -1;

    return 0;
}


static int
enter_file_in_map_and_list(int flags, parray_t *list, const char *name, where_t where) {
    name_type_t nt;

    switch ((nt = name_type(name))) {
	case NAME_ZIP: {
	    auto a = Archive::open(name, TYPE_ROM, where, 0);
	    if (a) {
		if ((flags & DO_LIST) && list) {
		    parray_push(list, xstrdup(a->name.c_str()));
		}
		a->close();
	    }
	    break;
	}

	case NAME_CHD: {
	    auto disk = Disk::from_file(name, 0);
	    if (disk) {
		if (flags & DO_MAP) {
		    enter_disk_in_map(disk.get(), where);
		}
		if ((flags & DO_LIST) && list) {
		    parray_push(list, xstrdup(disk->name.c_str()));
		}
	    }
	    break;
	}
	    
	case NAME_UNKNOWN:
	    /* ignore unknown files */
	    break;
    }

    return 0;
}
