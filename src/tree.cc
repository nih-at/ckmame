/*
  tree.c -- traverse tree of games to check
  Copyright (C) 1999-2015 Dieter Baron and Thomas Klausner

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "check.h"
#include "check_util.h"
#include "diagnostics.h"
#include "dbh.h"
#include "error.h"
#include "funcs.h"
#include "game.h"
#include "globals.h"
#include "sighandle.h"
#include "tree.h"


bool Tree::add(const std::string &game_name) {
    GamePtr game = db->read_game(game_name);
    
    if (!game) {
	return false;
    }
    
    auto tree = this;

    if (!game->cloneof[1].empty()) {
	tree = tree->add_node(game->cloneof[1], false);
    }
    if (!game->cloneof[0].empty()) {
        tree = tree->add_node(game->cloneof[0], false);
    }

    tree->add_node(game_name, true);

    return true;
}


bool Tree::recheck(const std::string &game_name) {
    if (game_name == name) {
        checked = false;
        return check;
    }

    for (auto it : children) {
        if (it.second->recheck(game_name)) {
            return true;
	}
    }

    return false;
}


bool Tree::recheck_games_needing(uint64_t size, const Hashes *hashes) {
    auto roms = db->read_file_by_hash(TYPE_ROM, hashes);
    if (roms.empty()) {
	return true;
    }

    auto ok = true;
    for (size_t i = 0; i < roms.size(); i++) {
        auto &rom = roms[i];

        auto game = db->read_game(rom.name);
        if (!game || game->roms.size() <= rom.index) {
            /* TODO: internal error: db inconsistency */
	    ok = false;
	    continue;
	}

        auto gr = &game->roms[rom.index];

        if (size == gr->size && hashes->compare(gr->hashes) == Hashes::MATCH && gr->where == FILE_INGAME) {
            recheck(game->name);
        }
    }

    return ok;
}


void Tree::traverse() {
    ArchivePtr archives[] = { NULL, NULL, NULL };
    ImagesPtr images[] = { std::make_shared<Images>(), std::make_shared<Images>(), std::make_shared<Images>() };

    for (auto it : children) {
        it.second->traverse_internal(archives, images);
    }
}

void Tree::traverse_internal(ArchivePtr *ancestor_archives, ImagesPtr *ancestor_images) {
    ArchivePtr archives[] = { NULL, ancestor_archives[0], ancestor_archives[1] };
    ImagesPtr images[] = { std::make_shared<Images>(), ancestor_images[0], ancestor_images[1] };
    
    ImagesPtr self_images;
    
    if (siginfo_caught) {
        print_info(name);
    }

    auto flags = ((check ? ARCHIVE_FL_CREATE : 0) | (check_integrity ? (ARCHIVE_FL_CHECK_INTEGRITY | db->hashtypes(TYPE_ROM)) : 0));
    
    auto full_name = findfile(name, TYPE_ROM, "");
    if (full_name.empty() && check) {
        full_name = make_file_name(TYPE_ROM, name, "");
    }
    if (!full_name.empty()) {
        archives[0] = Archive::open(full_name, TYPE_ROM, FILE_ROMSET, flags);
    }
    
    self_images = Images::from_directory(name, check_integrity);
    images[0] = self_images;
    
    if (check && !checked) {
        process(archives, images);
    }

    for (auto it : children) {
        it.second->traverse_internal(archives, images);
    }
}


Tree *Tree::add_node(const std::string &game_name, bool do_check) {
    auto it = children.find(game_name);
    
    if (it == children.end()) {
        auto child = std::make_shared<Tree>(game_name, do_check);
        children[game_name] = child;
        return child.get();
    }
    else {
        if (do_check) {
            it->second->check = true;
        }
        return it->second.get();
    }
}


void Tree::process(ArchivePtr *archives, ImagesPtr *images) {
    auto game = db->read_game(name);
    
    if (!game) {
	myerror(ERRDEF, "db error: %s not found", name.c_str());
        return;
    }

    Result res(game.get(), archives[0].get(), images[0].get());

    check_old(game.get(), &res);
    check_files(game.get(), archives, &res);
    check_archive(archives[0], game->name, &res);
    check_disks(game.get(), images, &res);
    check_images(images[0].get(), game->name.c_str(), &res);

    /* write warnings/errors for me */
    diagnostics(game.get(), archives[0], images[0].get(), &res);

    int ret = 0;

    if (fix_options & FIX_DO) {
	ret = fix_game(game.get(), archives[0].get(), images[0].get(), &res);
    }

    /* TODO: includes too much when rechecking */
    if (fixdat) {
	write_fixdat_entry(game.get(), &res);
    }

    if (ret != 1) {
	checked = true;
    }
}
