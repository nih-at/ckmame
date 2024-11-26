/*
  Fixdat.cc -- write fixdat
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

#include "Fixdat.h"

#include "globals.h"
#include "RomDB.h"
#include "util.h"

std::vector<Fixdat> Fixdat::fixdats;

void Fixdat::begin() {
    auto dats = db->read_dat();

    for (const auto& dat : dats) {
	fixdats.emplace_back(dat);
    }
}

void Fixdat::end() {
    fixdats.clear();
}

void Fixdat::write_entry(const Game *game, const Result *result) {
    fixdats[game->dat_no].write(game, result);
}

void Fixdat::write(const Game *game, const Result *result) {
    if (failed) {
	return;
    }
    if (!GS_HAS_MISSING(result->game)) {
	return;
    }

    auto gm = std::make_shared<Game>();
    gm->name = game->name;

    auto empty = true;
    
    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        for (size_t i = 0; i < game->files[ft].size(); i++) {
            auto &match = result->game_files[ft][i];
            auto &rom = game->files[ft][i];
            
            /* no use requesting zero byte files */
            if (rom.hashes.size == 0) {
                continue;
            }
            
            if (match.quality != Match::MISSING || rom.status == Rom::NO_DUMP || rom.where != FILE_INGAME) {
                continue;
            }
            
            gm->files[ft].push_back(rom);
            empty = false;
        }
    }

    if (!empty) {
	if (ensure_output()) {
	    output->game(gm);
	}
    }
}

bool Fixdat::ensure_output() {
    if (output) {
	return true;
    }

    if (failed) {
	return false;
    }

    auto fixdat_fname = "fixdat_" + dat.name + " (" + dat.version + ").dat";
    if (!configuration.fixdat_directory.empty()) {
	ensure_dir(configuration.fixdat_directory, false);
	fixdat_fname = configuration.fixdat_directory + "/" + fixdat_fname;
    }

    DatEntry de;

    de.name = "Fixdat for " + dat.name + " (" + dat.version + ")";
    de.description = "Fixdat by ckmame";
    de.version = format_time("%Y-%m-%d %H:%M:%S", time(nullptr));

    if ((output = OutputContext::create(OutputContext::FORMAT_DATAFILE_XML, fixdat_fname, 0)) == nullptr) {
	failed = true;
	return false;
    }

    if (!output->header(&de)) {
	output = nullptr;
	failed = true;
	return false;
    }

    return true;
}
