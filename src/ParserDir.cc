/*
  ParserDir.cc -- read info from zip archives
  Copyright (C) 2006-2014 Dieter Baron and Thomas Klausner

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

#include "ParserDir.h"

#include <algorithm>
#include <filesystem>

#include "Archive.h"
#include "Dir.h"
#include "globals.h"
#include "util.h"

bool ParserDir::parse() {
    lineno = 0;

    // TODO: set name from directory name?

    if (header_only) {
	return true;
    }

    try {
	Dir dir(directory_name, false);

	if (configuration.roms_zipped) {
            auto have_loose_chds = false;

	    for (const auto& entry : dir) {
	        auto filepath = entry.path();
                if (std::filesystem::is_directory(filepath)) {
                    auto dir_empty = true;
                    
                    {
                        auto images = Archive::open(filepath, TYPE_DISK, FILE_NOWHERE, 0);
                    
                        if (images && !images->is_empty()) {
                            dir_empty = false;
                            std::sort(images->files.begin(), images->files.end());
                            start_game(images->name, directory_name);
                            parse_archive(TYPE_DISK, images.get());
                        }
                    }
                    {
                        // TODO: Remove this ugly hack.
                        configuration.roms_zipped = false;
                        auto files = Archive::open(filepath, TYPE_ROM, FILE_NOWHERE, 0);
                        configuration.roms_zipped = true;

                        if (files && !files->is_empty()) {
                            dir_empty = false;
                            for (auto &file : files->files) {
                                auto extension = std::filesystem::path(file.name).extension();
                                
                                if (extension == ".chd") {
                                    continue;
                                }
                                else if (is_ziplike(file.name)) {
                                    auto a = Archive::open(filepath / file.name, TYPE_ROM, FILE_NOWHERE, 0);
                                    if (a) {
                                        auto name = a->name;
                                        if (!runtest) {
                                            auto extension = std::filesystem::path(name).extension();
                                            name = name.substr(0, name.length() - extension.string().length());
                                        }
                                        start_game(name, directory_name);
                                        parse_archive(TYPE_ROM, a.get());
                                        end_game();
                                    }
                                }
                                else {
                                    output.error("skipping unknown file '%s/%s'", filepath.c_str(), file.name.c_str());
                                }
                            }
                        }
                    }
                    
                    if (dir_empty) {
                        output.error("skipping empty directory '%s'", filepath.c_str());
                    }
                }
                else {
                    switch (name_type(entry)) {
                        case NAME_ZIP: {
                            /* TODO: handle errors */
                            auto a = Archive::open(filepath, TYPE_ROM, FILE_NOWHERE, 0);
                            if (a) {
                                auto name = a->name;
                                if (!runtest) {
                                    auto extension = std::filesystem::path(name).extension();
                                    name = name.substr(0, name.length() - extension.string().length());
                                }
                                start_game(name, directory_name);
                                parse_archive(TYPE_ROM, a.get());
                                end_game();
                            }
                            break;
                        }
                            
                            
                        case NAME_IMAGES:
                        case NAME_IGNORE:
                            // ignore
                            break;
                            
                        case NAME_UNKNOWN:
                            if (filepath.extension() == ".chd") {
                                if (runtest) {
                                    have_loose_chds = true;
                                }
                                else {
                                    output.error("skipping top level disk image '%s'", filepath.c_str());
                                }
                            }
                            else {
                                output.error("skipping unknown file '%s'", filepath.c_str());
                            }
                            break;
                    }
                }
        }

            end_game();

            if (have_loose_chds) {
                auto a = Archive::open_toplevel(directory_name, TYPE_DISK, FILE_NOWHERE, 0);

                if (a) {
                    start_game(".", "");
                    parse_archive(TYPE_DISK, a.get());
                    end_game();
                }

            }
	}
	else {
            auto have_loose_files = false;

	    for (const auto& entry : dir) {
                if (entry.is_directory()) {
                    /* TODO: handle errors */
                    auto a = Archive::open(entry.path(), TYPE_ROM, FILE_NOWHERE, 0);
                    if (a) {
                        start_game(a->name, directory_name);
                        parse_archive(TYPE_ROM, a.get());
                        end_game();
                    }
                }
                else {
                    if (entry.is_regular_file()) {
                        /* TODO: always include loose files, separate flag? */
                        if (options.full_archive_names) {
                            have_loose_files = true;
                        }
                        else {
                            output.error("found file '%s' outside of game subdirectory", entry.path().c_str());
                        }
                    }
                }
            }
            
            if (have_loose_files) {
                auto a = Archive::open_toplevel(directory_name, TYPE_ROM, FILE_NOWHERE, 0);
                
                if (a) {
                    start_game(".", "");
                    parse_archive(TYPE_ROM, a.get());
                    end_game();
                }
            }
        }
        
        eof();
    }
    catch (...) {
	return false;
    }

    return true;
}


bool ParserDir::parse_archive(filetype_t filetype, Archive *a) {
    std::string name;
    
    int wanted_hashtypes = filetype == TYPE_ROM ? hashtypes : Hashes::TYPE_ALL;

    for (size_t i = 0; i < a->files.size(); i++) {
        auto &file = a->files[i];

        if (filetype == TYPE_ROM) {
            a->file_ensure_hashes(i, wanted_hashtypes);
        }

        file_start(filetype);
        file_name(filetype, file.name);
        file_size(filetype, file.hashes.size);
        file_mtime(filetype, file.mtime);
        if (file.broken) {
            file_status(filetype, "baddump");
	}
        for (int ht = 1; ht <= Hashes::TYPE_MAX; ht <<= 1) {
            if ((wanted_hashtypes & ht) && file.hashes.has_type(ht)) {
                file_hash(filetype, ht, file.hashes.to_string(ht));
	    }
	}
        file_end(filetype);
    }

    return true;
}

void ParserDir::start_game(const std::string &name, const std::string &top_directory) {
    if (current_game == name) {
        return;
    }
    
    if (!current_game.empty()) {
        game_end();
    }
    
    game_start();
    game_name(top_directory.empty() ? name : name.substr(top_directory.length() + 1));
    current_game = name;
}

void ParserDir::end_game() {
    if (!current_game.empty()) {
        game_end();
    }
    current_game = "";
}
