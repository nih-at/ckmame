/*
  update_romdb.cc -- make sure RomDB is up to date
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

    std::vector<DatEntry> db_dat_list;

    try {
	auto existing_db = RomDB(configuration.rom_db, DBH_READ);
	db_dat_list = existing_db.read_dat();
    }
    catch (...) { }

    std::unordered_map<std::string, const DatEntry*> db_dats;

    for (const auto &db_dat : db_dat_list) {
	db_dats[db_dat.name] = &db_dat;
    }

    // TODO: remove duplicates from dats

    for (const auto &dat_name : configuration.dats) {
	auto it = db_dats.find(dat_name);
	auto fs_dats = repository.find_dats(dat_name);
	if (fs_dats.empty()) {
	    throw Exception("can't find dat '" + dat_name + "'");
	}
        else if (fs_dats.size() > 1) {
            throw Exception("multiple different dats found for '" + dat_name + "' with version '" + fs_dats[0].version + "'");
        }
	const auto& fs_dat = fs_dats[0];

	dats_to_use.push_back(fs_dat);

	if (it == db_dats.end()) {
	    output.message("%s (-> %s)", dat_name.c_str(), fs_dat.version.c_str());
	    up_to_date = false;
	    continue;
	}
	const auto& db_dat = it->second;

        if (fs_dat.version == db_dat->version && fs_dat.crc != db_dat->crc) {
            output.message("%s (%s %x -> %x)", dat_name.c_str(), db_dat->version.c_str(), db_dat->crc, fs_dat.crc);
            up_to_date = false;
        }
	else if (DatRepository::is_newer(fs_dat.version, db_dat->version)) {
	    output.message("%s (%s -> %s)", dat_name.c_str(), db_dat->version.c_str(), fs_dat.version.c_str());
	    up_to_date = false;
	}
    }

    // TODO: check that no additional dats are in db

    return up_to_date;
}


bool update_romdb(bool force) {
    if (configuration.dats.empty() || configuration.dat_directories.empty()) {
	return false;
    }

    std::vector<DatDB::DatInfo> dats_to_use;

    if (is_romdb_up_to_date(dats_to_use) && !force) {
	return false;
    }

    OutputContextPtr output;

    try {
	output = OutputContext::create(OutputContext::FORMAT_DB, configuration.rom_db, 0);

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

	    auto options = Parser::Options(dat.name);
	    if (!Parser::parse(source, {}, nullptr, output.get(), options)) {
		auto message = "can't parse '" + dat.file_name + "'";
		if (!dat.entry_name.empty()) {
		    message += "/" + dat.entry_name;
		}
		throw Exception(message);
	    }
	}

	auto ok = output->close();
	output.reset();
	if (!ok) {
	    throw Exception("can't write database");
	}
    }
    catch (std::exception &ex) {
	if (output) {
	    output->error_occurred();
	    output->close();
	}
	throw Exception(std::string(ex.what()));
    }

    return true;
}
