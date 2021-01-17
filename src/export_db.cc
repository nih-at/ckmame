/*
  export_db.c -- export games from db to output backend
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

#include <unordered_set>

#include "error.h"
#include "parse.h"
#include <stdlib.h>

int export_db(romdb_t *db, const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *out) {
    DatEntry de;

    if (out == NULL) {
	/* TODO: split into original dat files */
	return 0;
    }

    auto db_dat = db->read_dat();

    /* TODO: export detector */

    de.merge(dat, (db_dat.size() == 1 ? &db_dat[0] : NULL));
    out->header(&de);

    auto list = db->read_list(DBH_KEY_LIST_GAME);
    if (list.empty()) {
	myerror(ERRDEF, "db error reading game list");
	return -1;
    }

    for (size_t i = 0; i < list.size(); i++) {
        GamePtr game = db->read_game(list[i]);
        if (!game) {
	    /* TODO: error */
	    continue;
	}
        
        if (exclude.find(game->name) == exclude.end()) {
	    out->game(game);
        }
    }

    return 0;
}
