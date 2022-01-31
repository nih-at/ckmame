/*
DatRepository.cc -- find dat files in directories
Copyright (C) 2021 Dieter Baron and Thomas Klausner

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

#include "DatRepository.h"

#include <unordered_set>

#include "Dir.h"
#include "util.h"

DatRepository::DatRepository(const std::vector<std::string> &directories) {
    for (auto const &directory : directories) {
	auto db = std::make_shared<DatDB>(directory);
	dbs[directory] = db;

	update_directory(directory, db);
    }
}

DatRepository::~DatRepository() {
    for (auto const &pair : dbs) {
	auto const &db = pair.second;

	if (db->is_empty()) {
	    // TODO: close and remove db
	}
    }
}


std::optional<DatDB::DatInfo> DatRepository::find_dat(const std::string &name) {
    auto dat = DatDB::DatInfo();

    for (auto const &pair : dbs) {
	auto matches = pair.second->get_dats(name);

	for (const auto &match : matches) {
	    if (is_newer(dat.version, match.version)) {
		dat = match;
	    }
	}
    }

    if (dat.file_name.empty()) {
	return {};
    }
    return dat;
}


void DatRepository::update_directory(const std::string &directory, const DatDBPtr &db) {
    auto dir = Dir(directory, true);
    std::unordered_set<std::string> files;
    std::filesystem::path filepath;

    while ((filepath = dir.next()) != "") {
	if (directory == filepath || name_type(filepath) == NAME_IGNORE || !std::filesystem::is_regular_file(filepath)) {
	    continue;
	}

	auto file = filepath.string().substr(directory.size() + 1);

	files.insert(file);

	// TODO: parse file, enter results in db
    }

    auto db_files = db->list_files();

    for (const auto &file: db_files) {
	if (files.find(file) == files.end()) {
	    db->delete_file(file);
	}
    }
}
bool DatRepository::is_newer(const std::string& a, const std::string& b) {
    return a > b;
}
