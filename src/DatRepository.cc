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

#include <sys/stat.h>

#include <set>
#include <unordered_set>

#include "Dir.h"
#include "OutputContextHeader.h"
#include "Parser.h"
#include "ParserSourceFile.h"
#include "ParserSourceZip.h"
#include "util.h"
#include "Exception.h"
#include "globals.h"

DatRepository::DatRepository(const std::vector<std::string> &directories) {
    for (auto const &directory : directories) {
	if (!std::filesystem::is_directory(directory)) {
	    output.error("Warning: dat directory '%s' doesn't exist", directory.c_str());
	    continue;
	}

	DatDBPtr db;
	try {
	    db = std::make_shared<DatDB>(directory);
	}
	catch (...) {
	    std::filesystem::remove(std::filesystem::path(directory).append(DatDB::db_name));
	    try {
		db = std::make_shared<DatDB>(directory);
	    }
	    catch (...) {
		continue;
	    }
	}
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


std::vector<DatDB::DatInfo> DatRepository::find_dats(const std::string &name) {
    auto dats = std::vector<DatDB::DatInfo>{};
    std::string newest_version;

    for (auto const &pair : dbs) {
	auto matches = pair.second->get_dats(name);

	for (const auto &match : matches) {
	    if (match.version == newest_version) {
	        dats.emplace_back(match, pair.first + "/" + match.file_name); // TODO: use std::filesystem
	    }
	    else if (is_newer(match.version, newest_version)) {
	        dats.clear();
	        dats.emplace_back(match, pair.first + "/" + match.file_name); // TODO: use std::filesystem
	    }
	}
    }

    return dats;
}


std::vector<std::string> DatRepository::list_dats() {
    std::set<std::string> dats;

    for (const auto& pair : dbs) {
	for (const auto& dat : pair.second->list_dats()) {
	    dats.insert(dat);
	}
    }

    return {dats.begin(), dats.end()};
}

void DatRepository::update_directory(const std::string &directory, const DatDBPtr &db) {
    auto dir = Dir(directory, true);
    std::unordered_set<std::string> files;

    for (const auto& entry: dir) {
	try {
	    if (directory == entry.path() || name_type(entry) == NAME_IGNORE || !entry.is_regular_file()) {
		continue;
	    }

	    auto file = entry.path().string().substr(directory.size() + 1);

	    struct stat st{};

	    if (stat(entry.path().c_str(), &st) < 0) {
		continue;
	    }

	    files.insert(file);

	    time_t db_mtime;
	    size_t db_size;

	    if (db->get_last_change(file, &db_mtime, &db_size)) {
		if (db_mtime == st.st_mtime && db_size == static_cast<size_t>(st.st_size)) {
		    continue;
		}
		db->delete_file(file);
	    }

	    std::vector<DatDB::DatEntry> entries;

	    try {
		auto zip_archive = zip_open(entry.path().c_str(), 0, nullptr);
		if (zip_archive != nullptr) {
		    for (size_t index = 0; static_cast<int64_t>(index) < zip_get_num_entries(zip_archive, 0); index++) {
			try {
			    auto entry_name = zip_get_name(zip_archive, index, 0);
			    auto output = OutputContextHeader();

			    auto source = std::make_shared<ParserSourceZip>(entry.path(), zip_archive, entry_name);
                            auto parser_options = Parser::Options{{}, false};
			    auto parser = Parser::create(source, {}, nullptr, &output, parser_options);
			    if (parser) {
				if (parser->parse_header()) {
				    auto header = output.get_header();
				    entries.emplace_back(entry_name, header.name, header.version, header.crc);
				}
			    }
			}
			catch (Exception &ex) {

			}
		    }
		}
		else {
		    auto output = OutputContextHeader();

		    auto source = std::make_shared<ParserSourceFile>(entry.path());
                    auto parser_options = Parser::Options{{}, false};
		    auto parser = Parser::create(source, {}, nullptr, &output, parser_options);

		    if (parser) {
			if (parser->parse_header() && output.close()) {
			    auto header = output.get_header();
			    entries.emplace_back("", header.name, header.version, header.crc);
			}
		    }
		}
	    }
	    catch (Exception &ex) {
		// TODO: warn or ignore?
	    }
	    db->insert_file(file, st.st_mtime, st.st_size, entries);
	}
	catch (Exception &ex) {
	    output.error("can't process '%s': %s", entry.path().c_str(), ex.what());
	}
    }

    auto db_files = db->list_files();

    for (const auto &file: db_files) {
	if (files.find(file) == files.end()) {
	    db->delete_file(file);
	}
    }
}


bool DatRepository::is_newer(const std::string& a, const std::string& b) {
    if (b.empty()) {
	return true;
    }
    return a > b;
}
