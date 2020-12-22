/*
  fixdat.c -- write fixdat
  Copyright (C) 2012-2014 Dieter Baron and Thomas Klausner

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


#include "funcs.h"
#include "globals.h"
#include "match.h"
#include "match_disk.h"
#include "output.h"


void
write_fixdat_entry(const Game *game, const Result *res) {
    if (result_game(res) != GS_MISSING && result_game(res) != GS_PARTIAL) {
	return;
    }

    auto gm = std::make_shared<Game>();
    gm->name = game->name;

    for (size_t i = 0; i < game->roms.size(); i++) {
        auto &match = res->roms[i];
        auto &rom = game->roms[i];

	/* no use requesting zero byte files */
        if (rom.size == 0) {
	    continue;
        }

        if (match.quality != QU_MISSING || rom.status == STATUS_NODUMP || rom.where != FILE_INGAME) {
	    continue;
        }

        gm->roms.push_back(rom);
    }

    for (size_t i = 0; i < game->disks.size(); i++) {
        auto &match_disk = res->disks[i];
        auto &disk = game->disks[i];

        if (match_disk.quality != QU_MISSING || disk.status == STATUS_NODUMP) {
	    continue;
        }

        gm->disks.push_back(disk);
        auto &dm = gm->disks[gm->disks.size() - 1];
        dm.name = disk.name;
        dm.merge = disk.merge;
    }

    if (!gm->roms.empty() || !gm->disks.empty()) {
	fixdat->game(gm);
    }
}
