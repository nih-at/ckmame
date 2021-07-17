/*
  output-db.c -- write games to DB
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

#include "error.h"


struct fbh_context {
    sqlite3 *db;
    filetype_t ft;
};


OutputContextDb::OutputContextDb(const std::string &dbname, int flags) {
    std::filesystem::remove(dbname);
    db = std::make_unique<RomDB>(dbname, DBH_NEW);
}


OutputContextDb::~OutputContextDb() {
    close();
}


void OutputContextDb::familymeeting(Game *parent, Game *child) {
    if (!parent->cloneof[0].empty()) {
	/* tell child of his grandfather */
        child->cloneof[1] = parent->cloneof[0];
    }

    /* look for files in parent */
    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        for (size_t i = 0; i < child->files[ft].size(); i++) {
            auto &cr = child->files[ft][i];
            for (size_t j = 0; j < parent->files[ft].size(); j++) {
                auto &pr = parent->files[ft][j];
                if (cr.is_mergable(pr)) {
                    cr.where = static_cast<where_t>(pr.where + 1);
                    break;
                }
            }
            if (cr.where == FILE_INGAME && !cr.merge.empty()) {
                myerror(ERRFILE, "In game '%s': '%s': merged from '%s', but parent does not contain matching file", child->name.c_str(), cr.name.c_str(), cr.merge.c_str());
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
                myerror(ERRDEF, "internal database error: child %s not in database", lost_children[i].c_str());
                return false;
            }
            
            bool is_lost = true;
            
            auto parent = db->read_game(child->cloneof[0]);
            if (!parent) {
                myerror(ERRDEF, "inconsistency: %s has non-existent parent %s", child->name.c_str(), child->cloneof[0].c_str());
                
                /* remove non-existent cloneof */
                child->cloneof[0] = "";
                db->update_game_parent(child.get());
                is_lost = false;
            }
            else if (!lost(parent.get())) {
                /* parent found */
                familymeeting(parent.get(), child.get());
                is_lost = false;
            }
            
            if (!is_lost) {
                db->update_file_location(child.get());
                lost_children.erase(lost_children.begin() + static_cast<long>(i));
            }
        }
    }
        
    return true;
}


bool OutputContextDb::lost(Game *game) {
    if (game->cloneof[0].empty()) {
	return false;
    }

    return std::find(lost_children.begin(), lost_children.end(), game->name) != lost_children.end();
}


bool OutputContextDb::close() {
    auto ok = true;

    if (db) {
        db->write_dat(dat);

        if (!handle_lost()) {
            ok = false;
        }

        if (sqlite3_exec(db->db, sql_db_init_2, NULL, NULL, NULL) != SQLITE_OK) {
            ok = false;
        }

        db = NULL;
    }

    return ok;
}


bool OutputContextDb::detector(Detector *detector) {
    db->write_detector(*detector);

    return true;
}


bool OutputContextDb::game(GamePtr game) {
    auto g2 = db->read_game(game->name);
    
    if (g2) {
        myerror(ERRDEF, "duplicate game '%s' skipped", game->name.c_str());
	return false;
    }

    game->dat_no = static_cast<unsigned int>(dat.size() - 1);

    if (!game->cloneof[0].empty()) {
        auto parent = db->read_game(game->cloneof[0]);
        if (!parent || lost(parent.get())) {
            lost_children.push_back(game->name);
        }
        else {
            familymeeting(parent.get(), game.get());
            /* TODO: check error */
        }
    }

    db->write_game(game.get());

    return true;
}


bool OutputContextDb::header(DatEntry *entry) {
    dat.push_back(*entry);
 
    return true;
}
