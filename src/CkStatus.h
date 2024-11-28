/*
    Dumpgame.h -- print info about game (from data base)
    Copyright (C) 2022 Dieter Baron and Thomas Klausner

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

#ifndef CKSTATUS_H
#define CKSTATUS_H

#include <set>

#include "Command.h"
#include "Result.h"
#include "StatusDB.h"

namespace std {
template <>
struct hash<std::vector<uint8_t>> {
  std::size_t operator()(const std::vector<unsigned char>& vec) const {
    std::size_t hash = 0;
    for (auto& byte : vec) {
      hash ^= std::hash<uint8_t>{}(byte) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
  }
};
}


class CkStatus : public Command {
  public:
    CkStatus();

    void global_setup(const ParsedCommandline &commandline) override;
    bool execute(const std::vector<std::string> &arguments) override;
    bool cleanup() override;

  private:
    enum Special {
        ALL_MISSING,
        CHANGES,
        CORRECT,
        CORRECT_MIA,
        MISSING,
        RUNS,
        SUMMARY
    };

    class RunDiff {
      public:
        RunDiff(int run_id1, int run_id2): run_id1(run_id1), run_id2(run_id2) {}

        void compute();

      private:
        class GameChecksum {
          public:
            std::string name;
            std::vector<uint8_t> checksum;

            bool operator<(const GameChecksum &other) const {return name < other.name ;}
        };

        class GameDiff {
          public:
            std::optional<StatusDB::GameInfo> old_info;
            std::optional<StatusDB::GameInfo> new_info;
        };

        static bool is_complete(GameStatus status) {return status == GS_CORRECT || status == GS_CORRECT_MIA;}
        void insert_run(bool old);

        int run_id1;
        int run_id2;
        std::vector<GameChecksum> games;
        std::unordered_map<std::vector<uint8_t>, GameDiff> diffs;
    };

    std::set<Special> specials;
    std::optional<int> run_id;
    std::optional<int> run_id2;

    int get_run_id();
    int get_run_id2();
    void list_games(GameStatus status);
    void list_games(GameStatus status1, GameStatus status2);
    void list_games(GameStatus status1, GameStatus status2, GameStatus status3);
    void list_games(GameStatus status1, GameStatus status2, GameStatus status3, GameStatus status4);
    void list_games(GameStatus status1, GameStatus status2, GameStatus status3, GameStatus status4, GameStatus status5, GameStatus status6);
    void list_games(const std::vector<std::string> &games);
    void list_runs();
    void list_summary();
    void print_changes();
};

#endif // CKSTATUS_H
