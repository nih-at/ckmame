/*
CkMameCache.cc -- collection of CkmameDBs.
Copyright (C) 1999-2022 Dieter Baron and Thomas Klausner

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

#include "CkmameCache.h"

#include <filesystem>

#include "error.h"
#include "globals.h"
#include "util.h"
#include "Exception.h"
#include "Dir.h"

CkmameCachePtr ckmame_cache;

CkmameCache::CkmameCache() :
    extra_delete_list(std::make_shared<DeleteList>()),
    needed_delete_list(std::make_shared<DeleteList>()),
    superfluous_delete_list(std::make_shared<DeleteList>()),
    extra_list_done(false),
    extra_map_done(false),
    needed_map_done(false) {
}

bool CkmameCache::close_all() {
    auto ok = true;

    for (auto &directory : cache_directories) {
	if (directory.db) {
	    bool empty = directory.db->is_empty();
	    std::string filename = sqlite3_db_filename(directory.db->db, "main");

	    directory.db = nullptr;
	    if (empty) {
		std::error_code ec;
		std::filesystem::remove(filename);
		if (ec) {
		    myerror(ERRDEF, "can't remove empty database '%s': %s", filename.c_str(), ec.message().c_str());
		    ok = false;
		}
	    }
	}
	directory.initialized = false;
    }

    return ok;
}


CkmameDBPtr CkmameCache::get_db_for_archive(const std::string &name) {
    for (auto &directory : cache_directories) {
	if (name.compare(0, directory.name.length(), directory.name) == 0 && (name.length() == directory.name.length() || name[directory.name.length()] == '/')) {
	    if (!directory.initialized) {
		directory.initialized = true;
		if (!configuration.fix_romset) {
		    std::error_code ec;
		    if (!std::filesystem::exists(directory.name, ec)) {
			return nullptr; /* we won't write any files, so DB would remain empty */
		    }
		}
		if (!ensure_dir(directory.name, false)) {
		    return nullptr;
		}

		auto dbname = directory.name + '/' + CkmameDB::db_name;

		try {
		    directory.db = std::make_shared<CkmameDB>(dbname, directory.name);
		}
		catch (std::exception &e) {
		    myerror(ERRDB, "can't open rom directory database for '%s': %s", directory.name.c_str(), e.what());
		    return nullptr;
		}
	    }
	    return directory.db;
	}
    }

    return nullptr;
}


void CkmameCache::register_directory(const std::string &directory_name) {
    std::string name;

    if (directory_name.empty()) {
	errno = EINVAL;
	myerror(ERRDEF, "directory_name can't be empty");
	throw Exception();
    }

    if (directory_name[directory_name.length() - 1] == '/') {
	name = directory_name.substr(0, directory_name.length() - 1);
    }
    else {
	name = directory_name;
    }

    for (auto &directory : cache_directories) {
	auto length = std::min(name.length(), directory.name.length());

	if (name.compare(0, length, directory.name) == 0 && (name.length() == length || name[length] == '/') && (directory.name.length() == length || directory.name[length] == '/')) {
	    if (directory.name.length() != name.length()) {
		myerror(ERRDEF, "can't cache in directory '%s' and its parent '%s'", (directory.name.length() < name.length() ? name.c_str() : directory.name.c_str()), (directory.name.length() < name.length() ? directory.name.c_str() : name.c_str()));
		throw Exception();
	    }
	    return;
	}
    }

    cache_directories.emplace_back(name);
}


void CkmameCache::ensure_extra_maps(bool do_map, bool do_list) {
    if (extra_map_done) {
	do_map = false;
    }
    if (extra_list_done) {
	do_list = false;
    }

    if (!do_map && !do_list) {
	return;
    }

    /* Opening the archives will register them in the map. */
    if (do_map) {
	extra_map_done = true;

	for (auto &entry : superfluous_delete_list->archives) {
	    auto file = entry.name;
	    switch ((name_type(file))) {
	    case NAME_IMAGES:
	    case NAME_ZIP: {
		auto a = Archive::open(file, entry.filetype, FILE_SUPERFLUOUS, 0);
		// TODO: loose: add loose files in directory
		break;
	    }

	    default:
		// TODO: loose: add loose top level file
		break;
	    }
	}

	auto filetype = configuration.roms_zipped ? TYPE_DISK : TYPE_ROM;
	auto a = Archive::open_toplevel(configuration.rom_directory, filetype, FILE_SUPERFLUOUS, 0);
    }

    for (const auto &directory : configuration.extra_directories) {
	enter_dir_in_map_and_list(do_list, extra_delete_list, directory, FILE_EXTRA);
    }

    if (do_list) {
	extra_delete_list->sort_archives();
    }
}


void CkmameCache::ensure_needed_maps() {
    if (needed_map_done) {
	return;
    }

    needed_map_done = true;
    needed_delete_list = std::make_shared<DeleteList>();

    enter_dir_in_map_and_list(false, needed_delete_list, configuration.saved_directory, FILE_NEEDED);
}


bool
CkmameCache::enter_dir_in_map_and_list(bool do_list, const DeleteListPtr &list, const std::string &directory_name, where_t where) {
    bool ret;
    if (configuration.roms_zipped) {
	ret = enter_dir_in_map_and_list_zipped(do_list, list, directory_name, where);
    }
    else {
	ret = enter_dir_in_map_and_list_unzipped(do_list, list, directory_name, where);
    }

    if (ret && do_list) {
	/* clean up cache db: remove archives no longer in file system */
	auto dbh = get_db_for_archive(directory_name);
	if (dbh) {
	    auto list_db = dbh->list_archives();
	    if (!list_db.empty()) {
		list->sort_archives();

		for (const auto &entry : list_db) {
		    std::string name = directory_name;
		    if (entry.name != ".") {
			name += '/' + entry.name;
		    }
		    if (!std::binary_search(list->archives.begin(), list->archives.end(), ArchiveLocation(name, entry.filetype))) {
			dbh->delete_archive(name, entry.filetype);
		    }
		}
	    }
	}
    }

    return ret;
}


bool CkmameCache::enter_dir_in_map_and_list_unzipped(bool do_list, const DeleteListPtr &list, const std::string &directory_name, where_t where) {
    try {
	Dir dir(directory_name, false);
	std::filesystem::path filepath;

	while (!(filepath = dir.next()).empty()) {
	    if (name_type(filepath) == NAME_IGNORE) {
		continue;
	    }
	    if (std::filesystem::is_directory(filepath)) {
		auto a = Archive::open(filepath, TYPE_ROM, where, 0);
		if (a) {
		    if (do_list) {
			list->add(a.get());
		    }
		    a->close();
		}
	    }
	}

	auto a = Archive::open_toplevel(directory_name, TYPE_ROM, where, 0);
	if (a) {
	    if (do_list) {
		list->add(a.get());
	    }
	}
    }
    catch (...) {
	return false;
    }

    return true;
}


bool CkmameCache::enter_dir_in_map_and_list_zipped(bool do_list, const DeleteListPtr &list, const std::string &dir_name, where_t where) {
    try {
	Dir dir(dir_name, true);
	std::filesystem::path filepath;

	while (!(filepath = dir.next()).empty()) {
	    enter_file_in_map_and_list(do_list, list, filepath, where);
	}

	auto a = Archive::open_toplevel(dir_name, TYPE_DISK, where, 0);
	if (a) {
	    if (do_list) {
		list->add(a.get());
	    }
	}
    }
    catch (...) {
	return false;
    }

    return true;
}


bool CkmameCache::enter_file_in_map_and_list(bool do_list, const DeleteListPtr &list, const std::string &name, where_t where) {
    name_type_t nt;

    switch ((nt = name_type(name))) {
    case NAME_IMAGES:
    case NAME_ZIP: {
	auto a = Archive::open(name, nt == NAME_ZIP ? TYPE_ROM : TYPE_DISK, where, 0);
	if (a) {
	    if (do_list) {
		list->add(a.get());
	    }
	    a->close();
	}
	break;
    }

    case NAME_IGNORE:
    case NAME_UNKNOWN:
	// TODO: loose: add unknown files?
	break;
    }

    return true;
}


void CkmameCache::used(Archive *a, size_t index) const {
    FileLocation fl(a->name + (a->contents->flags & ARCHIVE_FL_TOP_LEVEL_ONLY ? "/" : ""), a->filetype, index);

    switch (a->where) {
    case FILE_NEEDED:
	needed_delete_list->entries.push_back(fl);
	break;

    case FILE_SUPERFLUOUS:
	superfluous_delete_list->entries.push_back(fl);
	break;

    case FILE_EXTRA:
	if (configuration.move_from_extra) {
	    extra_delete_list->entries.push_back(fl);
	}
	break;

    default:
	break;
    }
}
