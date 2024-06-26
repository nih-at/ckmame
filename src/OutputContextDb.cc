/*
  OutputContextDb.cc -- write games to DB
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

#include "OutputContextDb.h"

#include <algorithm>
#include <filesystem>

#include "file_util.h"
#include "globals.h"


struct fbh_context {
    sqlite3 *db;
    filetype_t ft;
};


OutputContextDb::OutputContextDb(const std::string &dbname, int flags) :
									 file_name(dbname),
									 ok(true) {
    temp_file_name = file_name + "-mkmamedb";
    if (configuration.use_temp_directory) {
	auto tmpdir = getenv("TMPDIR");
	std::filesystem::path basename = temp_file_name;
	basename = basename.filename();
	temp_file_name = std::string(tmpdir ? tmpdir : "/tmp") + "/" + basename.string();
    }
    temp_file_name = make_unique_name(temp_file_name, "");

    db = std::make_unique<RomDB>(temp_file_name, DBH_NEW);
}


OutputContextDb::~OutputContextDb() {
    try {
        close();
    }
    catch (...) { }
}


void OutputContextDb::familymeeting(Game *parent, Game *child) {
    if (!parent->cloneof[0].empty()) {
	/* tell child of his grandfather */
        child->cloneof[1] = parent->cloneof[0];
    }
    
    auto grand_parent = child->cloneof[1].empty() ? nullptr : db->read_game(child->cloneof[1]);

    /* look for files in parent */
    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        for (auto &cr : child->files[ft]) {
            for (const auto &pr : parent->files[ft]) {
                if (cr.is_mergable(pr)) {
                    cr.where = static_cast<where_t>(pr.where + 1);
                    break;
                }
            }
            if (grand_parent != nullptr && cr.where == FILE_INGAME) {
                for (const auto &pr : grand_parent->files[ft]) {
                    if (cr.is_mergable(pr)) {
                        cr.where = FILE_IN_GRAND_CLONEOF;
                        break;
                    }
                }
            }
             
            if (cr.where == FILE_INGAME && !cr.merge.empty()) {
                output.file_error("In game '%s': '%s': merged from '%s', but ancestors don't contain matching file", child->name.c_str(), cr.name.c_str(), cr.merge.c_str());
            }
        }
    }
}


bool OutputContextDb::handle_lost() {
    while (!lost_children.empty()) {
        for (size_t i = 0; i < lost_children.size(); i++) {
            /* get current lost child from database, get parent,
             look if parent is still lost, if not, do child */
            auto child = db->read_game(lost_children[i]);
            if (!child) {
                output.error("internal database error: child %s not in database", lost_children[i].c_str());
                return false;
            }
            
            bool is_lost = true;

            auto parent_name = get_game_name(child->cloneof[0]);
            auto parent = db->read_game(parent_name);
            if (!parent) {
                output.error("inconsistency: %s has non-existent parent %s", child->name.c_str(), parent_name.c_str());
                
                /* remove non-existent cloneof */
                child->cloneof[0] = "";
                db->update_game_parent(child.get());
                is_lost = false;
            }
            else if (!lost(parent.get())) {
                /* parent found */
                if (child->cloneof[0] != parent_name) {
                    child->cloneof[0] = parent_name;
                    db->update_game_parent(child.get());
                }
                familymeeting(parent.get(), child.get());
                is_lost = false;
            }
            
            if (!is_lost) {
                db->update_file_location(child.get());
                lost_children.erase(lost_children.begin() + static_cast<long>(i));
            }
        }
    }

    renamed_games.clear();
        
    return true;
}


bool OutputContextDb::lost(Game *game) {
    if (game->cloneof[0].empty()) {
	return false;
    }

    return std::find(lost_children.begin(), lost_children.end(), game->name) != lost_children.end();
}


bool OutputContextDb::close() {
    if (db) {
	// TODO: don't write stuff if !ok
        db->write_dat(dat);

        if (!handle_lost()) {
            ok = false;
        }

        db->init2();

        db = nullptr;

	if (ok) { // TODO: and no previous errors
	    rename_or_move(temp_file_name, file_name);
	}
	else {
	    std::filesystem::remove(temp_file_name);
	}
    }

    return ok;
}


bool OutputContextDb::detector(Detector *detector) {
    db->write_detector(*detector);

    return true;
}


bool OutputContextDb::game(GamePtr game, const std::string &original_name) {
    if (!original_name.empty()) {
        renamed_games[original_name] = game->name;
    }
    auto g2 = db->read_game(game->name);

    if (g2) {
	std::string name;
	size_t n = 1;
	while (true) {
	    name = game->name + " (" + std::to_string(n) + ")";
	    if (db->read_game(name) == nullptr) {
		break;
	    }
	    n += 1;
	}
	output.error("warning: duplicate game '%s', renamed to '%s'", game->name.c_str(), name.c_str());
	game->name = name;
    }

    game->dat_no = static_cast<unsigned int>(dat.size() - 1);

    if (!game->cloneof[0].empty()) {
        auto parent_name = get_game_name(game->cloneof[0]);
        auto parent = db->read_game(parent_name);
        if (!parent || lost(parent.get())) {
            lost_children.push_back(game->name);
        }
        else {
            game->cloneof[0] = parent_name;
            familymeeting(parent.get(), game.get());
            /* TODO: check error */
        }
    }

    db->write_game(game.get());

    return true;
}


bool OutputContextDb::header(DatEntry *entry) {
    handle_lost(); // from previous dat

    dat.push_back(*entry);
 
    return true;
}


std::string OutputContextDb::get_game_name(const std::string &original_name) {
    auto it = renamed_games.find(original_name);
    if (it == renamed_games.end()) {
        return original_name;
    }
    else {
        return it->second;
    }
}
