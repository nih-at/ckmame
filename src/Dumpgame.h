/*
    Dumpgame.h -- print info about game (from data base)
    Copyright (C) 2022-2024 Dieter Baron and Thomas Klausner

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

#ifndef DUMPGAME_H
#define DUMPGAME_H

#include <set>

#include "Command.h"
#include "Game.h"
#include "DatEntry.h"

class Dumpgame : public Command {
  public:
    Dumpgame();

    void global_setup(const ParsedCommandline &commandline) override;
    bool execute(const std::vector<std::string> &arguments) override;
    bool global_cleanup() override;

  private:
    enum Special {
        DATS,
        DETECTOR,
        DISKS,
        GAMES,
        HASH_TYPES,
        MIA,
        SUMMARY
    };
    bool brief_mode;
    bool find_checksum;
    bool first;

    std::set<std::string> arguments;
    std::vector<Hashes> checksum_arguments;
    std::set<Special> specials;
    std::vector<bool> found;

    static bool dump_dats();
    static void dump_detector();
    bool dump_game(const std::string& name) const;
    static void dump_hash_types();
    static bool dump_list(int key);
    static void dump_stats();
    static std::string format_checksums(const Hashes *hashes);
    static std::string format_dat(const DatEntry &de);
    static std::string format_hash_types(int ht);
    static bool is_pattern(const std::string& string);
    static void print_clones(const GamePtr& game);
    static void print_diskline(Rom *disk);
    static void print_match(const GamePtr& game, filetype_t ft, size_t i);
    static void print_matches(const Hashes *hash);
    static void print_romline(Rom *rom);
};

#endif // DUMPGAME_H
