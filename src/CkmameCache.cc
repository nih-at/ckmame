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

#include <algorithm>
#include <filesystem>

#include "Dir.h"
#include "Exception.h"
#include "RomDB.h"
#include "globals.h"
#include "sighandle.h"
#include "util.h"

CkmameCachePtr ckmame_cache;

CkmameCache::CkmameCache() :
    extra_delete_list(std::make_shared<DeleteList>()),
    needed_delete_list(std::make_shared<DeleteList>()),
    superfluous_delete_list(std::make_shared<DeleteList>()),
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
		    output.error("can't remove empty database '%s': %s", filename.c_str(), ec.message().c_str());
		    ok = false;
		}
	    }
	}
	directory.initialized = false;
    }

    return ok;
}


CkmameDBPtr CkmameCache::get_db_for_archive(const std::string &name) {
    auto directory = get_directory_for_archive(name);

    if (directory) {
        return directory->db;
    }
    return nullptr;
}

std::string CkmameCache::get_directory_name_for_archive(const std::string &name) {
    auto directory = get_directory_for_archive(name);

    if (directory) {
        return directory->name;
    }
    return "";
}

const CkmameCache::CacheDirectory* CkmameCache::get_directory_for_archive(const std::string &name) {
    for (auto &directory : cache_directories) {
	if (name.compare(0, directory.name.length(), directory.name) == 0 && (name.length() == directory.name.length() || name[directory.name.length()] == '/')) {
            directory.initialize();
            if (directory.db) {
                return &directory;
            }
            else {
                return nullptr;
            }
	}
    }

    return nullptr;
}

void CkmameCache::CacheDirectory::initialize() {
    if (initialized) {
        return;
    }
    initialized = true;
    if (!configuration.fix_romset) {
        std::error_code ec;
        if (!std::filesystem::exists(name, ec)) {
            return; /* we won't write any files, so DB would remain empty */
        }
    }
    if (!ensure_dir(name, false)) {
        return;
    }

    try {
        db = std::make_shared<CkmameDB>(name, where);
    }
    catch (std::exception& e) {
        db = {};
        output.error_database("can't open rom directory database for '%s': %s", name.c_str(), e.what());
    }
}

void CkmameCache::register_directory(const std::string &directory_name, where_t where) {
    std::string name;

    if (directory_name.empty()) {
	errno = EINVAL;
	output.error("directory_name can't be empty");
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
		output.error("can't cache in directory '%s' and its parent '%s'", (directory.name.length() < name.length() ? name.c_str() : directory.name.c_str()), (directory.name.length() < name.length() ? directory.name.c_str() : name.c_str()));
		throw Exception();
	    }
	    return;
	}
    }

    // TODO: check that same directory isn't registerd with different where.

    cache_directories.emplace_back(name, where);
}


void CkmameCache::ensure_extra_maps() {
    if (extra_map_done) {
	return;
    }

    // TODO: this is a hack, to be replaced when we rework the delete lists.
    // Get superfluous files in ROM directory into ckmamedb.
    cache_directories[0].initialize();
    if (cache_directories[0].db) {
        cache_directories[0].db->refresh();
    }

    /* Opening the archives will register them in the map. */
    extra_map_done = true;

    for (auto &entry : superfluous_delete_list->archives) {
	auto file = entry.name;
	switch ((name_type(file))) {
	case NAME_IMAGES:
	case NAME_ZIP: {
            if (siginfo_caught) {
                print_info("currently scanning '" + file + "'");
            }
	    auto a = Archive::open(file, entry.filetype, FILE_SUPERFLUOUS, 0);
	    // TODO: loose: add loose files in directory
	    break;
	}

	default:
	    // TODO: loose: add loose top level file
	    break;
	}
    }

    if (siginfo_caught) {
        print_info("currently scanning '" + configuration.rom_directory + "'");
    }
    auto filetype = configuration.roms_zipped ? TYPE_DISK : TYPE_ROM;
    auto a = Archive::open_toplevel(configuration.rom_directory, filetype, FILE_SUPERFLUOUS, 0);


    for (const auto &directory : configuration.extra_directories) {
	enter_dir_in_map_and_list(extra_delete_list, directory, FILE_EXTRA);
    }

    extra_delete_list->sort_archives();
}


void CkmameCache::ensure_needed_maps() {
    if (needed_map_done) {
	return;
    }

    needed_map_done = true;
    needed_delete_list = std::make_shared<DeleteList>();

    enter_dir_in_map_and_list(needed_delete_list, configuration.saved_directory, FILE_NEEDED);
}


bool CkmameCache::enter_dir_in_map_and_list(const DeleteListPtr &list, const std::string &directory_name, where_t where) {
    bool ret;
    if (configuration.roms_zipped) {
	ret = enter_dir_in_map_and_list_zipped(list, directory_name, where);
    }
    else {
	ret = enter_dir_in_map_and_list_unzipped(list, directory_name, where);
    }

    if (ret) {
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


bool CkmameCache::enter_dir_in_map_and_list_unzipped(const DeleteListPtr &list, const std::string &directory_name, where_t where) {
    try {
	Dir dir(directory_name, false);
	std::filesystem::path filepath;

	while (!(filepath = dir.next()).empty()) {
	    if (name_type(filepath) == NAME_IGNORE) {
		continue;
	    }
	    if (std::filesystem::is_directory(filepath)) {
                if (siginfo_caught) {
                    print_info("currently scanning '" + filepath.string() + "'");
                }
		auto a = Archive::open(filepath, TYPE_ROM, where, 0);
		if (a) {
		    list->add(a.get());
		    a->close();
		}
	    }
	}

        if (siginfo_caught) {
            print_info("currently scanning '" + directory_name + "'");
        }
	auto a = Archive::open_toplevel(directory_name, TYPE_ROM, where, 0);
	if (a) {
	    list->add(a.get());
	}
    }
    catch (...) {
	return false;
    }

    return true;
}


bool CkmameCache::enter_dir_in_map_and_list_zipped(const DeleteListPtr &list, const std::string &dir_name, where_t where) {
    try {
	Dir dir(dir_name, true);
	std::filesystem::path filepath;

	while (!(filepath = dir.next()).empty()) {
	    enter_file_in_map_and_list(list, filepath, where);
	}

        if (siginfo_caught) {
            print_info("currently scanning '" + dir_name + "'");
        }
	auto a = Archive::open_toplevel(dir_name, TYPE_DISK, where, 0);
	if (a) {
	    list->add(a.get());
	}
    }
    catch (...) {
	return false;
    }

    return true;
}


bool CkmameCache::enter_file_in_map_and_list(const DeleteListPtr &list, const std::string &name, where_t where) {
    name_type_t nt;

    switch ((nt = name_type(name))) {
    case NAME_IMAGES:
    case NAME_ZIP: {
        if (siginfo_caught) {
            print_info("currently scanning '" + name + "'");
        }
	auto a = Archive::open(name, nt == NAME_ZIP ? TYPE_ROM : TYPE_DISK, where, 0);
	if (a) {
	    list->add(a.get());
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


void CkmameCache::used(Archive *a, size_t index) {
    FileLocation fl(a->name + (a->contents->flags & ARCHIVE_FL_TOP_LEVEL_ONLY ? "/" : ""), a->filetype, index);

    switch (a->where) {
    case FILE_NEEDED:
	needed_delete_list->entries.push_back(fl);
	break;

    case FILE_ROMSET: {
        if (a->name == configuration.rom_directory) {
            superfluous_delete_list->entries.push_back(fl);
            break;
        }
        auto name = a->name.substr(configuration.rom_directory.size() + 1);
        if (!db->game_exists(name)) {
            superfluous_delete_list->entries.push_back(fl);
        }
        break;
    }

    case FILE_SUPERFLUOUS:
	superfluous_delete_list->entries.push_back(fl);
	break;

    case FILE_EXTRA: {
        auto directory = get_directory_name_for_archive(a->name);

        if (configuration.extra_directory_move_from_extra(directory)) {
            extra_delete_list->entries.push_back(fl);
        }
        break;
    }

    default:
	break;
    }
}
std::vector<CkmameDB::FindResult> CkmameCache::find_file(filetype_t filetype, size_t detector_id, const FileData& rom) {
    auto results = std::vector<CkmameDB::FindResult>();

    for (auto& cache_directory: cache_directories) {
        cache_directory.initialize();
        if (cache_directory.db) {
            cache_directory.db->find_file(filetype, detector_id, rom, results);
        }
    }

//    printf("searching for file '%s', got %lu results\n", rom.name.c_str(), results.size());
    return results;
}
