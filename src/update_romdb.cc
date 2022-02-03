/*
  udpdate_romdb.cc -- make sure RomDB is up to date
  Copyright (C) 2022 Dieter Baron and Thomas Klausner

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

#include "update_romdb.h"

#include <filesystem>

#include "DatRepository.h"
#include "Exception.h"
#include "globals.h"
#include "OutputContext.h"
#include "RomDB.h"
#include "ParserSourceZip.h"
#include "ParserSourceFile.h"
#include "Parser.h"

static bool is_romdb_up_to_date(std::vector<DatDB::DatInfo> &dats_to_use) {
    auto repository = DatRepository(configuration.dat_directories);

    auto up_to_date = true;

    std::vector<DatEntry> db_dats;

    try {
	auto existing_db = RomDB(configuration.rom_db, DBH_READ);
	db_dats = existing_db.read_dat();
    }
    catch (...) { }

    std::unordered_map<std::string, std::string> db_versions;

    for (const auto &db_dat : db_dats) {
	db_versions[db_dat.name] = db_dat.version;
    }

    // TODO: remove duplicates from dats

    for (const auto &dat_name : configuration.dats) {
	auto it = db_versions.find(dat_name);
	auto fs_dat_maybe = repository.find_dat(dat_name);
	if (!fs_dat_maybe.has_value()) {
	    throw Exception("can't find dat '" + dat_name + "'");
	}
	const auto& fs_dat = fs_dat_maybe.value();

	dats_to_use.push_back(fs_dat);

	if (it == db_versions.end()) {
	    if (configuration.verbose) { // TODO: different config setting
		printf("%s (not in database)\n", dat_name.c_str());
	    }
	    up_to_date = false;
	    continue;
	}
	const auto& db_version = it->second;

	if (DatRepository::is_newer(fs_dat.version, db_version)) {
	    if (configuration.verbose) { // TODO: different config setting
		printf("%s (%s -> %s)\n", dat_name.c_str(), db_version.c_str(), fs_dat.version.c_str());
	    }
	    up_to_date = false;
	}
    }

    // TODO: check that no additional dats are in db

    return up_to_date;
}


void update_romdb() {
    if (configuration.dats.empty() || configuration.dat_directories.empty()) {
	return;
    }

    std::vector<DatDB::DatInfo> dats_to_use;

    if (is_romdb_up_to_date(dats_to_use)) {
	return;
    }

    // TODO: implement use_temp_name
    // TODO: use proper temp name
    auto temp_name = configuration.rom_db + ".xxx";

    try {
	auto output = OutputContext::create(OutputContext::FORMAT_DB, temp_name, 0);

	for (const auto &dat : dats_to_use) {
	    ParserSourcePtr source;

	    if (dat.entry_name.empty()) {
		source = std::make_shared<ParserSourceFile>(dat.file_name);
	    }
	    else {
		int error_code;
		auto zip_archive = zip_open(dat.file_name.c_str(), 0, &error_code);
		if (zip_archive == nullptr) {
		    zip_error_t error;
		    zip_error_init_with_code(&error, error_code);
		    auto message = "can't open '" + dat.file_name + "': " + zip_error_strerror(&error);
		    zip_error_fini(&error);
		    throw Exception(message);
		}
		source = std::make_shared<ParserSourceZip>(dat.file_name, zip_archive, dat.entry_name);
	    }
	    if (!Parser::parse(source, {}, nullptr, output.get(), 0)) {
		auto message = "can't parse '" + dat.file_name + "'";
		if (!dat.entry_name.empty()) {
		    message += "/" + dat.entry_name;
		}
		throw Exception(message);
	    }
	}

	output->close();

	std::filesystem::rename(temp_name, configuration.rom_db);
    }
    catch (std::exception &ex) {
	std::filesystem::remove(temp_name);
	throw ex;
    }
}
