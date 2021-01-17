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
#include <filesystem>

#include <sys/param.h>
#include <sys/stat.h>

#include "dbh_cache.h"
#include "delete_list.h"
#include "Dir.h"
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

static bool enter_dir_in_map_and_list(int, std::vector<std::string> &, const char *, bool recursive, where_t);
static bool enter_dir_in_map_and_list_unzipped(int, std::vector<std::string> &, const char *, bool recursive, where_t);
static bool enter_dir_in_map_and_list_zipped(int, std::vector<std::string> &, const char *, bool recursive, where_t);
static bool enter_file_in_map_and_list(int flags,  std::vector<std::string> &, const char *name, where_t where);


void
ensure_extra_maps(int flags) {
    name_type_t nt;

    if ((flags & (DO_MAP | DO_LIST)) == 0) {
	return;
    }

    if (((maps_done & EXTRA_MAPS) && !(flags & DO_LIST)) || (!extra_list.empty() && !(flags & DO_MAP))) {
	return;
    }

    if (!(maps_done & EXTRA_MAPS) && (flags & DO_MAP)) {
	maps_done |= EXTRA_MAPS;
	extra_delete_list = std::make_shared<DeleteList>();
	superfluous_delete_list = std::make_shared<DeleteList>();
    }

    if (flags & DO_MAP) {
	memdb_ensure();
	for (size_t i = 0; i < superfluous.size(); i++) {
	    auto file = superfluous[i];
	    switch ((nt = name_type(file.c_str()))) {
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

    for (size_t i = 0; i < search_dirs.size(); i++) {
	enter_dir_in_map_and_list(flags, extra_list, search_dirs[i].c_str(), true, FILE_EXTRA);
    }

    if (flags & DO_LIST) {
	std::sort(extra_list.begin(), extra_list.end());
    }
}


void
ensure_needed_maps(void) {
    std::vector<std::string> dummy;
    if (maps_done & NEEDED_MAPS)
	return;

    maps_done |= NEEDED_MAPS;
    needed_delete_list = std::make_shared<DeleteList>();

    enter_dir_in_map_and_list(DO_MAP, dummy, needed_dir, true, FILE_NEEDED);
}


std::string findfile(const std::string &name, filetype_t what, const std::string &game_name) {
    if (what == TYPE_FULL_PATH) {
	if (std::filesystem::exists(name)) {
	    return name;
	}
	else {
	    return "";
	}
    }

    auto fn = make_file_name(what, name, game_name);
    if (std::filesystem::exists(fn)) {
	return fn;
    }
    if (what == TYPE_DISK) {
	/* strip off ".chd" */
	fn = fn.substr(0, fn.size() - 4);
	if (std::filesystem::exists(fn)) {
	    return fn;
	}
    }

    return "";
}


std::string make_file_name(filetype_t ft, const std::string &name, const std::string &game_name) {
    std::string result;

    result = std::string(get_directory()) + "/";
    if (ft == TYPE_DISK) {
	result += game_name + "/";
    }
    result += name;
    if (ft == TYPE_DISK) {
	result += ".chd";
    } else {
	if (!roms_unzipped) {
	    result += ".zip";
	}
    }

    return result;
}


static bool
enter_dir_in_map_and_list(int flags,  std::vector<std::string> &list, const char *directory_name, bool recursive, where_t where) {
    bool ret;
    if (roms_unzipped) {
	ret = enter_dir_in_map_and_list_unzipped(flags | DO_LIST, list, directory_name, recursive, where);
    }
    else {
	ret = enter_dir_in_map_and_list_zipped(flags | DO_LIST, list, directory_name, recursive, where);
    }

    if (ret) {
	/* clean up cache db: remove archives no longer in file system */
	DB *dbh = dbh_cache_get_db_for_archive(directory_name);
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
		    if (std::find(list.begin(), list.end(), name) == list.end()) {
			dbh_cache_delete_by_name(dbh, name.c_str());
		    }
		}
	    }
	}
    }

    return ret;
}

static bool
enter_dir_in_map_and_list_unzipped(int flags, std::vector<std::string> &list, const char *directory_name, bool recursive, where_t where) {
    try {
	 Dir dir(directory_name, false);
	 std::filesystem::path filepath;

	 while ((filepath = dir.next()) != "") {
	     if (filepath.filename() == DBH_CACHE_DB_NAME) {
		 continue;
	     }
	     if (std::filesystem::is_directory(filepath)) {
		 auto a = Archive::open(filepath, TYPE_ROM, where, 0);
		 if (a) {
		     if (flags & DO_LIST) {
			 list.push_back(filepath);
		     }
		     a->close();
		 }

	     }
	 }

	 auto a = Archive::open_toplevel(directory_name, TYPE_ROM, where, 0);
	 if (a) {
	     if (flags & DO_LIST) {
		 list.push_back(a->name);
	     }
	     a->close();
	 }
    }
    catch (...) {
	return false;
    }

    return true;
}


static bool
enter_dir_in_map_and_list_zipped(int flags, std::vector<std::string> &list, const char *dir_name, bool recursive, where_t where) {
    try {
	 Dir dir(dir_name, recursive);
	 std::filesystem::path filepath;

	 while ((filepath = dir.next()) != "") {
	     enter_file_in_map_and_list(flags, list, filepath.c_str(), where);
	 }
    }
    catch (...) {
	return false;
    }

    return true;
}


bool
enter_disk_in_map(const Disk *disk, where_t where) {
    sqlite3_stmt *stmt;

    if ((stmt = memdb->get_statement(DBH_STMT_MEM_INSERT_FILE)) == NULL) {
	return false;
    }

    if (sqlite3_bind_int64(stmt, 1, disk->id) != SQLITE_OK || sqlite3_bind_int(stmt, 2, TYPE_DISK) != SQLITE_OK || sqlite3_bind_int(stmt, 3, 0) != SQLITE_OK || sqlite3_bind_int(stmt, 4, 0) != SQLITE_OK || sqlite3_bind_int(stmt, 5, where) != SQLITE_OK || sqlite3_bind_null(stmt, 6) != SQLITE_OK || sq3_set_hashes(stmt, 7, &disk->hashes, 1) != SQLITE_OK || sqlite3_step(stmt) != SQLITE_DONE) {
	return false;
    }

    return true;
}


static bool
enter_file_in_map_and_list(int flags, std::vector<std::string> &list, const char *name, where_t where) {
    name_type_t nt;

    switch ((nt = name_type(name))) {
	case NAME_ZIP: {
	    auto a = Archive::open(name, TYPE_ROM, where, 0);
	    if (a) {
		if (flags & DO_LIST) {
		    list.push_back(a->name);
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
		if (flags & DO_LIST) {
		    list.push_back(disk->name);
		}
	    }
	    break;
	}

	case NAME_UNKNOWN:
	    /* ignore unknown files */
	    break;
    }

    return true;
}
