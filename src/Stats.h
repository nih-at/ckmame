#ifndef _HAD_STATS_H
#define _HAD_STATS_H

/*
 Stats.h -- store stats of the ROM set
 Copyright (C) 2018-2020 Dieter Baron and Thomas Klausner

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

#include "FileData.h"
#include "Match.h"
#include "Result.h"
#include "types.h"

class StatsFiles {
 public:
    StatsFiles() : files_total(0), files_good(0), bytes_total(0), bytes_good(0) { }
    uint64_t files_total;
    uint64_t files_good;
    uint64_t bytes_total;
    uint64_t bytes_good;
};

class Stats {
 public:
    Stats() : games_total(0), games_good(0), games_partial(0) {}
    uint64_t games_total;
    uint64_t games_good;
    uint64_t games_partial;
    StatsFiles files[TYPE_MAX];

    void add_game(GameStatus status);
    void add_rom(enum filetype type, const FileData *rom, Match::Quality status);
    void print(FILE *f, bool total_only);

 private:
    void add_file(enum filetype type, uint64_t size, Match::Quality status);
};

#endif /* _HAD_STATS_H */
