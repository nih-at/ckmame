/*
  superfluous.c -- check for unknown file in rom directories
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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

#include "superfluous.h"

#include <algorithm>

#include "Archive.h"
#include "Dir.h"
#include "error.h"
#include "globals.h"
#include "RomDB.h"
#include "util.h"


static void list_non_chds(std::vector<std::string> &found, const std::string &dirname);

std::vector<std::string> list_directory(const std::string &dirname, const std::string &dbname) {
    std::vector<std::string> result;
    std::vector<std::string> known_games;

    if (!dbname.empty()) {
        known_games = db->read_list(DBH_KEY_LIST_GAME);
	if (known_games.empty()) {
            myerror(ERRDEF, "list of games not found in database '%s'", dbname.c_str());
            exit(1);
        }
    }

    std::sort(known_games.begin(), known_games.end());

    bool have_toplevel = false;
    
    try {
        Dir dir(dirname, false);
        std::filesystem::path filepath;
        
        while ((filepath = dir.next()) != "") {
            if (name_type(filepath) == NAME_CKMAMEDB) {
                continue;
            }
            
            bool known = false;
            
            if (std::filesystem::is_directory(filepath)) {
                auto filename = filepath.filename();
                known = std::binary_search(known_games.begin(), known_games.end(), filename);;
                
                if (!roms_unzipped) {
                    list_non_chds(result, filepath);
                }
            }
            else {
                if (!roms_unzipped) {
                    auto ext = filepath.extension();
                    
                    if (ext == ".zip") {
                        auto stem = filepath.stem();
                        known = std::binary_search(known_games.begin(), known_games.end(), stem);
                    }
                    else if (ext == ".chd") {
                        // TODO: I don't think we want top level CHDs in this list.
                        known = true;
                        have_toplevel = true;
                    }
                }
                else {
                    // TODO: Don't list top level files for unzipped in list either.
                    known = true;
                    have_toplevel = true;
                }
            }
            
            if (!known) {
                result.push_back(filepath);
            }
        }
        
        if (have_toplevel) {
            result.push_back(dirname + "/");
        }
    }
    catch (...) {
	return result;
    }

    std::sort(result.begin(), result.end());

    return result;
}


void print_superfluous(std::vector<std::string> &files) {
    if (files.empty()) {
	return;
    }

    std::vector<std::string> extra_files;

    for (auto &file : files) {
        if (file[file.length() - 1] == '/') {
            auto a = Archive::open(file, roms_unzipped ? TYPE_ROM : TYPE_DISK, FILE_NOWHERE, 0);

            if (a) {
                for (auto &f : a->files) {
                    extra_files.push_back(file + f.filename());
                }
            }
        }
        else {
            extra_files.push_back(file);
        }
    }

    if (!extra_files.empty()) {
        std::sort(extra_files.begin(), extra_files.end());
        printf("Extra files found:\n");
        for (auto & file : extra_files) {
            printf("%s\n", file.c_str());
        }
    }
}


static void list_non_chds(std::vector<std::string> &found, const std::string &dirname) {
    try {
	Dir dir(dirname, true);
	std::filesystem::path filepath;

	while ((filepath = dir.next()) != "") {
            if (filepath.extension() != ".chd") {
                found.push_back(filepath);
            }
	}
    }
    catch (...) {
	return;
    }
}
