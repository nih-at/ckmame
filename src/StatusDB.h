#ifndef HAD_STATUS_DB_H
#define HAD_STATUS_DB_H

/*
  StatusDB.h -- sqlite3 data base for status of last runs.
  Copyright (C) 2024 Dieter Baron and Thomas Klausner

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

#include "DB.h"
#include "DatEntry.h"
#include "Result.h"

class StatusDB: public DB {
  public:
    enum Statement {
        CLEANUP_DAT,
        CLEANUP_GAME,
        DELETE_RUN,
        FIND_DAT,
        INSERT_DAT,
        INSERT_GAME,
        INSERT_RUN,
        LIST_RUNS,
        QUERY_GAME,
        QUERY_GAME_BY_STATUS,
        QUERY_GAME_STATI,
        QUERY_RUN_STATUS_COUNTS
    };

    class Run {
      public:
        int64_t run_id{};
        time_t date{};
    };

    class Game {
      public:
        int64_t dat_id{};
        std::string name;
        std::vector<uint8_t> checksum;
        GameStatus status;
    };

    class Status {
      public:
        std::string name;
        GameStatus status;
    };

    class StatusCount {
      public:
        GameStatus status;
        int count;
    };

    static const DBFormat format;
    static const std::string db_name;

    StatusDB(const std::string &name, int mode) : DB(format, name, mode) {}
    ~StatusDB() override = default;

    void delete_runs(time_t oldest, int count);
    [[nodiscard]] std::vector<Run> list_runs();
    [[nodiscard]] std::vector<Game> get_games(int64_t run_id);
    [[nodiscard]] std::vector<std::string> get_games_by_status(int64_t run_id, GameStatus status);
    [[nodiscard]] std::vector<Status> get_game_stati(int64_t run_id);
    [[nodiscard]] std::vector<StatusCount> get_run_status_counts(int64_t run_id);
    [[nodiscard]] int64_t insert_dat(const DatEntry& dat);
    void insert_game(int64_t run_id, const Game& game);
    [[nodiscard]] int64_t insert_run(time_t date);

  protected:
    [[nodiscard]] std::string get_query(int name, bool parameterized) const override;

  private:
    static std::unordered_map<int, std::string> queries;

    DBStatement *get_statement(Statement name) { return get_statement_internal(name); }

};


#endif // HAD_STATUS_DB_H
