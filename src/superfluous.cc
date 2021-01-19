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

#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Dir.h"
#include "error.h"
#include "globals.h"
#include "romdb.h"
#include "types.h"
#include "util.h"

static void list_game_directory(std::vector<std::string> &found, const char *dirname, bool dir_known);

std::vector<std::string>
list_directory(const std::string &dirname, const std::string &dbname) {
    std::vector<std::string> result;
    std::vector<std::string> list;

    if (dbname != "") {
        list = db->read_list(DBH_KEY_LIST_GAME);
	if (list.empty()) {
            myerror(ERRDEF, "list of games not found in database '%s'", dbname.c_str());
            exit(1);
        }
    }

    try {
	 Dir dir(dirname, false);
	 std::filesystem::path filepath;

	 while ((filepath = dir.next()) != "") {
	     if (filepath.filename() == DBH_CACHE_DB_NAME) {
		 continue;
	     }

	     bool known = false;

	     if (std::filesystem::is_directory(filepath)) {
		 if (roms_unzipped) {
		     known = (std::find(list.begin(), list.end(), filepath.filename()) != list.end());
		 }
		 else {
		     bool dir_known = (std::find(list.begin(), list.end(), filepath.filename()) != list.end());
		     list_game_directory(result, filepath.c_str(), dir_known);
		     known = true; /* we don't want directories in superfluous list (I think) */
		 }
	     }
	     else {
		 auto ext = filepath.extension();
		 if (ext != "") {
		     if (!roms_unzipped && ext == ".zip") {
			 known = (std::find(list.begin(), list.end(), filepath.stem()) != list.end());
		     }
		 }
	     }

	     if (!known) {
		 result.push_back(filepath);
	     }
	 }
    }
    catch (...) {
	return result;
    }

    std::sort(result.begin(), result.end());

    return result;
}


void
print_superfluous(std::vector<std::string> &files) {
    if (files.empty()) {
	return;
    }

    printf("Extra files found:\n");

    for (size_t i = 0; i < files.size(); i++) {
	printf("%s\n", files[i].c_str());
    }
}


static void
list_game_directory(std::vector<std::string> &found, const char *dirname, bool dir_known) {
    GamePtr game;

    auto component = std::filesystem::path(dirname).filename();
    if (dir_known) {
        game = db->read_game(component);
    }

    try {
	Dir dir(dirname, false);
	std::filesystem::path filepath;

	while ((filepath = dir.next()) != "") {
	    bool known = false;
	    if (game) {
		if (filepath.extension() == ".chd") {
		    for (size_t i = 0; i < game->disks.size(); i++) {
			if (game->disks[i].name == filepath.stem()) {
			    known = true;
			    break;
			}
		    }
		}
	    }

	    if (!known) {
		found.push_back(filepath);
	    }
	}
    }
    catch (...) {
	return;
    }
}
