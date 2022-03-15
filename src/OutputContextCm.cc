/*
  output-cm.c -- write games to clrmamepro dat files
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

#include "OutputContextCm.h"

#include <algorithm>
#include <cerrno>
#include <cinttypes>
#include <cstring>

#include "util.h"
#include "globals.h"


static struct {
    bool operator()(GamePtr a, GamePtr b) const {
        return a->name < b->name;
    }
} cmp_game;

typedef struct output_context_cm output_context_cm_t;


OutputContextCm::OutputContextCm(const std::string &fname_, int flags_) : fname(fname_) {
    if (fname.empty()) {
        f = make_shared_stdout();
	fname = "*stdout*";
    }
    else {
	f = make_shared_file(fname, "w");
	if (!f) {
            output.error("cannot create '%s': %s", fname.c_str(), strerror(errno));
            throw std::exception();
	}
    }
}

OutputContextCm::~OutputContextCm() {
    close();
}

bool OutputContextCm::close() {
    if (f == nullptr) {
        return false;
    }
    
    std::sort(games.begin(), games.end(), cmp_game);

    for (auto &game : games) {
        write_game(game.get());
    }

    auto ok = true;
    
    if (f) {
        ok = fflush(f.get()) == 0;
    }
    
    f = nullptr;

    return ok;
}


bool OutputContextCm::game(GamePtr game) {
    games.push_back(game);

    return true;
}


bool OutputContextCm::header(DatEntry *dat) {
    fputs("clrmamepro (\n", f.get());
    cond_print_string(f, "\tname ", dat->name, "\n");
    cond_print_string(f, "\tdescription ", (dat->description.empty() ? dat->name : dat->description), "\n");
    cond_print_string(f, "\tversion ", dat->version, "\n");
    fputs(")\n\n", f.get());

    return true;
}


bool OutputContextCm::write_game(Game *game) {
    fputs("game (\n", f.get());
    cond_print_string(f, "\tname ", game->name, "\n");
    cond_print_string(f, "\tdescription ", game->description.empty() ? game->name : game->description, "\n");
    cond_print_string(f, "\tcloneof ", game->cloneof[0], "\n");
    cond_print_string(f, "\tromof ", game->cloneof[0], "\n");
    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        for (auto &rom : game->files[ft]) {
            fprintf(f.get(), "\t%s ( ", ft == TYPE_ROM ? "rom" : "disk");
            cond_print_string(f, "name ", rom.name, " ");
            if (rom.where != FILE_INGAME) {
                cond_print_string(f, "merge ", rom.merge.empty() ? rom.name : rom.merge, " ");
            }
            fprintf(f.get(), "size %" PRIu64 " ", rom.hashes.size);
            cond_print_hash(f, "crc ", Hashes::TYPE_CRC, &rom.hashes, " ");
            cond_print_hash(f, "md5 ", Hashes::TYPE_MD5, &rom.hashes, " ");
            cond_print_hash(f, "sha1 ", Hashes::TYPE_SHA1, &rom.hashes, " ");
            cond_print_string(f, "flags ", rom.status_name(), " ");
            fputs(")\n", f.get());
        }
    }
    fputs(")\n\n", f.get());

    return true;
}
