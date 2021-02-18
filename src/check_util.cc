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

#include "check_util.h"

#include <algorithm>
#include <filesystem>

#include <sys/param.h>
#include <sys/stat.h>

#include "dbh_cache.h"
#include "delete_list.h"
#include "Dir.h"
#include "error.h"
#include "globals.h"
#include "memdb.h"
#include "sq_util.h"
#include "util.h"

const std::string needed_dir = "needed";   /* TODO: proper value */
const std::string unknown_dir = "unknown"; /* TODO: proper value */

std::vector<std::string> search_dirs;

std::vector<std::string> superfluous;

#define EXTRA_MAPS 0x1
#define NEEDED_MAPS 0x2

static int maps_done = 0;

static bool enter_dir_in_map_and_list(int, std::vector<std::string> &, const std::string &, bool recursive, where_t);
static bool enter_dir_in_map_and_list_unzipped(int, std::vector<std::string> &, const std::string &, bool recursive, where_t);
static bool enter_dir_in_map_and_list_zipped(int, std::vector<std::string> &, const std::string &, bool recursive, where_t);
static bool enter_file_in_map_and_list(int flags,  std::vector<std::string> &, const std::string &name, where_t where);


void ensure_extra_maps(int flags) {
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

    /* Opening the archives will register them in the map. */
    if (flags & DO_MAP) {
	memdb_ensure();
	for (size_t i = 0; i < superfluous.size(); i++) {
	    auto file = superfluous[i];
	    switch ((nt = name_type(file))) {
		case NAME_IMAGES:
		case NAME_ZIP: {
		    auto filetype = nt == NAME_ZIP ? TYPE_ROM : TYPE_DISK;
		    auto a = Archive::open(file, filetype, FILE_SUPERFLUOUS, 0);
		    break;
		}

	    default:
		/* ignore unknown files */
		break;
	    }
	}

	auto filetype = roms_unzipped ? TYPE_ROM : TYPE_DISK;
	auto a = Archive::open_toplevel(get_directory(), filetype, FILE_SUPERFLUOUS, 0);
    }

    for (size_t i = 0; i < search_dirs.size(); i++) {
	enter_dir_in_map_and_list(flags, extra_list, search_dirs[i], true, FILE_EXTRA);
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


std::string findfile(filetype_t filetype, const std::string &name) {
    if (filetype == TYPE_FULL_PATH) {
	if (std::filesystem::exists(name)) {
	    return name;
	}
	else {
	    return "";
	}
    }

    auto fn = make_file_name(filetype, name);
    if (std::filesystem::exists(fn)) {
	return fn;
    }

    return "";
}


std::string make_file_name(filetype_t filetype, const std::string &name) {
    std::string result;

    result = get_directory() + "/" + name;
    if (filetype == TYPE_ROM && !roms_unzipped) {
	result += ".zip";
    }

    return result;
}


static bool enter_dir_in_map_and_list(int flags,  std::vector<std::string> &list, const std::string &directory_name, bool recursive, where_t where) {
    bool ret;
    if (roms_unzipped) {
	ret = enter_dir_in_map_and_list_unzipped(flags | DO_LIST, list, directory_name, recursive, where);
    }
    else {
	ret = enter_dir_in_map_and_list_zipped(flags | DO_LIST, list, directory_name, recursive, where);
    }

    if (ret) {
	/* clean up cache db: remove archives no longer in file system */
	auto dbh = dbh_cache_get_db_for_archive(directory_name);
	if (dbh) {
	    auto list_db = dbh_cache_list_archives(dbh.get());
	    if (!list_db.empty()) {
		std::sort(list.begin(), list.end());

		for (auto entry_name : list_db) {
		    std::string name = directory_name;
		    if (entry_name != ".") {
			name += '/' + entry_name;
		    }
		    if (!std::binary_search(list.begin(), list.end(), name)) {
			dbh_cache_delete_by_name(dbh.get(), name);
		    }
		}
	    }
	}
    }

    return ret;
}

static bool enter_dir_in_map_and_list_unzipped(int flags, std::vector<std::string> &list, const std::string &directory_name, bool recursive, where_t where) {
    try {
	 Dir dir(directory_name, false);
	 std::filesystem::path filepath;

	 while (!(filepath = dir.next()).empty()) {
	     if (name_type(filepath) == NAME_CKMAMEDB) {
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
	 }
    }
    catch (...) {
	return false;
    }

    return true;
}


static bool
enter_dir_in_map_and_list_zipped(int flags, std::vector<std::string> &list, const std::string &dir_name, bool recursive, where_t where) {
    try {
	Dir dir(dir_name, recursive);
	std::filesystem::path filepath;
	
	while (!(filepath = dir.next()).empty()) {
	    enter_file_in_map_and_list(flags, list, filepath, where);
	}
	
	auto a = Archive::open_toplevel(dir_name, TYPE_DISK, where, 0);
	if (a) {
	    if (flags & DO_LIST) {
		list.push_back(a->name);
	    }
	}
    }
    catch (...) {
	return false;
    }

    return true;
}


static bool enter_file_in_map_and_list(int flags, std::vector<std::string> &list, const std::string &name, where_t where) {
    name_type_t nt;

    switch ((nt = name_type(name))) {
	case NAME_IMAGES:
	case NAME_ZIP: {
	    auto a = Archive::open(name, nt == NAME_ZIP ? TYPE_ROM : TYPE_DISK, where, 0);
	    if (a) {
		if (flags & DO_LIST) {
		    list.push_back(a->name);
		}
		a->close();
	    }
	    break;
	}

	case NAME_CKMAMEDB:
	case NAME_UNKNOWN:
	    /* ignore unknown files */
	    break;
    }

    return true;
}
