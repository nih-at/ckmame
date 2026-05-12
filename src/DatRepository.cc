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
#include "Exception.h"
#include "OutputContextHeader.h"
#include "Parser.h"
#include "ParserSourceFile.h"
#include "ParserSourceZip.h"
#include "globals.h"
#include "util.h"

DatRepository::DatRepository(const std::vector<std::string>& directories) {
    for (auto const& directory : directories) {
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
    for (auto const& pair : dbs) {
        auto const& db = pair.second;

        if (db->is_empty()) {
            // TODO: close and remove db
        }
    }
}

/**
 * Find newest dat.
 * 
 * @param name Name of the dat to find.
 * @param allow_empty If true, empty dats are allowed. Otherwise, the newest non-empty dat is used, but a warning is printed if there is a newer empty dat.
 * @return Info about the dat.
 * @throw Exception if dat can't be found.
 */
DatDB::DatInfo DatRepository::find_dat(const std::string& name, bool allow_empty) {
    std::optional<DatDB::DatInfo> newest_empty_dat;
    std::optional<DatDB::DatInfo> newest_dat;

    for (const auto& [directory, db] : dbs) {
        auto matches = db->get_dats(name);

        for (const auto& match : matches) {
            if (!allow_empty && match.empty) {
                if (!newest_empty_dat || match > *newest_empty_dat) {
                    newest_empty_dat = match;
                }
            }
            else {
                if (newest_empty_dat && match > *newest_empty_dat) {
                    newest_empty_dat = std::nullopt;
                }

                if (!newest_dat || match > *newest_dat) {
                    newest_dat = DatDB::DatInfo(match, std::filesystem::path(directory) / match.file_name);
                }
                else {
                    break;
                }
            }
        }
    }

    if (newest_empty_dat) {
        if (!newest_dat) {
            throw Exception("only empty dats found for '" + name + "'");
        }
        if (newest_empty_dat > *newest_dat) {
            if (newest_empty_dat->version != newest_dat->version) {
                output.error("warning: newest dat for '%s' with version '%s' is empty, using older dat with version '%s'", name.c_str(),   newest_empty_dat->version.c_str(), newest_dat->version.c_str()); 
            }
            else {
                output.error("warning: newest dat for '%s' with version '%s' is empty, using older dat with same version", name.c_str(), newest_empty_dat->version.c_str()); // TODO: include mtime in message
            }
        }
    }

    if (!newest_dat) {
        throw Exception("can't find dat '" + name + "'");
    }

    return *newest_dat;
}


/**
 * Get list of all dat names in all directories.
 * 
 * @return Sorted list of all dat names in all directories.
 * @throw Exception if there is an error accessing the database.
 */
std::vector<std::string> DatRepository::list_dats() {
    std::set<std::string> dats;

    for (const auto& pair : dbs) {
        for (const auto& dat : pair.second->list_dats()) {
            dats.insert(dat);
        }
    }

    return {dats.begin(), dats.end()};
}

void DatRepository::update_directory(const std::string& directory, const DatDBPtr& db) {
    auto dir = Dir(directory, true);
    std::unordered_set<std::string> files;

    for (const auto& entry : dir) {
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
                                    entries.emplace_back(entry_name, header.name, header.version, header.crc, output.empty);
                                }
                            }
                        }
                        catch (Exception& ex) {
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
                            entries.emplace_back("", header.name, header.version, header.crc, output.empty);
                        }
                    }
                }
            }
            catch (Exception& ex) {
                // TODO: warn or ignore?
            }
            db->insert_file(file, st.st_mtime, st.st_size, entries);
        }
        catch (Exception& ex) {
            output.error("can't process '%s': %s", entry.path().c_str(), ex.what());
        }
    }

    auto db_files = db->list_files();

    for (const auto& file : db_files) {
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
