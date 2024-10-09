/*
StatDBRun.cc -- 

Copyright (C) Dieter Baron

The authors can be contacted at <accelerate@tpau.group>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. The names of the authors may not be used to endorse or promote
  products derived from this software without specific prior
  written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS
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

#include "StatusDBRun.h"

StatusDBRun::StatusDBRun(std::shared_ptr<StatusDB> db_, RomDB *romdb): db(std::move(db_)), romdb(romdb) {
    run_id = db->insert_run(time(nullptr));
    dats = romdb->read_dat();
}

void StatusDBRun::insert_game_status(const Game& game, const GameStatus& game_status) {
    if (!db) {
        return;
    }
    db->insert_game(run_id, game, get_dat_id(game.dat_no), game_status);
}

int64_t StatusDBRun::get_dat_id(size_t dat_no) {
    auto it = dat_ids.find(dat_no);
    if (it != dat_ids.end()) {
        return it->second;
    }

    auto id = db->find_dat(dats[dat_no]);
    if (id < 0) {
        id = db->insert_dat(dats[dat_no]);
    }

    dat_ids[dat_no] = id;
    return id;
}
