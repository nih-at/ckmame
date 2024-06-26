/*
StatusDB.cc -- sqlite3 data base for status of last runs.
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

#include "StatusDB.h"
#include "Exception.h"

const std::string StatusDB::db_name = ".ckmame-status.db";

const DB::DBFormat StatusDB::format = {
    0x3,
    1,
    "\
create table run (\n\
    run_id integer primary key autoincrement,\n\
    date integer not null\n\
);\n\
create table dat (\n\
    dat_id integer primary key,\n\
    name text,\n\
    version text\n\
);\n\
create table game (\n\
    run_id integer not null\n\
    dat_id integer not null\n\
    name text not null\n\
    checksum binary not null\n\
    status integer not null\n\
);",
    {}
};

std::unordered_map<int, std::string> StatusDB::queries = {
    { CLEANUP_DAT, "delete from dat where dat_id not in (select distinct(dat_id) from file)"},
    { CLEANUP_GAME, "delete from game where run_id not in (select run_id from run)" },
    { DELETE_RUN, "delete from run where date < :date and run_id not in (select run_id from run order by date descending limit :count"},
    { FIND_DAT, "select dat_id from dat where name = :name and version = :version" },
    { INSERT_DAT, "inesrt into dat (name, version) values (:name, :version)" },
    { INSERT_GAME, "insert into game (run_id, dat_id, name, checksum, status) values (:run_id, :dat_id, :name, :checksum, :status" },
    { INSERT_RUN, "insert into run (date) values (:date)" },
    { LIST_RUNS, "select run_id, date from run order by date descending" },
    { QUERY_GAME, "select dat_id, name, checksum, status from game where run_id = :run_id" },
    { QUERY_GAME_BY_STATUS, "select name from game where run_id = :run, status = :status order by name"},
    { QUERY_GAME_STATI, "select name, status from game where run_id = :run order by name"},
    { QUERY_RUN_STATUS_COUNTS, "select status, count(*) as status_count from game where run_id = :run_id group by status" },
};

std::string StatusDB::get_query(int name, bool parameterized) const {
    if (parameterized) {
        return "";
    }
    else {
        auto it = queries.find(static_cast<Statement>(name));
        if (it == queries.end()) {
            return "";
        }
        return it->second;
    }
}

void StatusDB::delete_runs(time_t oldest, int count) {
    auto stmt = get_statement(DELETE_RUN);

    stmt->set_int64("oldest", static_cast<int64_t>(oldest));
    stmt->set_int("count", count);

    if (!stmt->step()) {
        return;
    }

    stmt = get_statement(CLEANUP_GAME);
    if (!stmt->step()) {
        return;
    }

    stmt = get_statement(CLEANUP_DAT);
    stmt->step();
}

std::vector<StatusDB::Game> StatusDB::get_games(int64_t run_id) {
    auto stmt = get_statement(QUERY_GAME);

    stmt->set_int64("run_id", run_id);

    std::vector<Game> games;

    while (stmt->step()) {
        Game game;

        game.dat_id = stmt->get_int("dat_id");
        game.name = stmt->get_string("name");
        game.checksum = stmt->get_blob("checksum");
        game.status = static_cast<GameStatus>(stmt->get_int("status"));

        games.push_back(game);
    }

    return games;
}

std::vector<StatusDB::Run> StatusDB::list_runs() {
    auto stmt = get_statement(QUERY_GAME);

    std::vector<Run> runs;

    while (stmt->step()) {
        Run run;

        run.run_id = stmt->get_int64("run_id");
        run.date = static_cast<time_t>(stmt->get_int64("date"));

        runs.push_back(run);
    }

    return runs;
}

std::vector<std::string> StatusDB::get_games_by_status(int64_t run_id, GameStatus status) {
    auto stmt = get_statement(QUERY_GAME_BY_STATUS);

    stmt->set_int64("run_id", run_id);
    stmt->set_int("status", status);

    std::vector<std::string> games;

    while (stmt->step()) {
        games.push_back(stmt->get_string("name"));
    }

    return games;
}

std::vector<StatusDB::Status> StatusDB::get_game_stati(int64_t run_id) {
    auto stmt = get_statement(QUERY_GAME_STATI);

    stmt->set_int64("run_id", run_id);

    std::vector<Status> stati;

    while (stmt->step()) {
        Status status;

        status.name = stmt->get_string("name");
        status.status = static_cast<GameStatus>(stmt->get_int("status"));

        stati.push_back(status);
    }

    return stati;
}

std::vector<StatusDB::StatusCount> StatusDB::get_run_status_counts(int64_t run_id) {
    auto stmt = get_statement(QUERY_RUN_STATUS_COUNTS);

    stmt->set_int64("run_id", run_id);

    std::vector<StatusCount> counts;

    while (stmt->step()) {
        StatusCount count;

        count.status = static_cast<GameStatus>(stmt->get_int("status"));
        count.count = stmt->get_int("status_count");

        counts.push_back(count);
    }

    return counts;
}

int64_t StatusDB::insert_dat(const DatEntry& dat) {
    auto stmt = get_statement(FIND_DAT);

    stmt->set_string("name", dat.name);
    stmt->set_string("version", dat.version);

    if (stmt->step()) {
        return stmt->get_int64("dat_id");
    }

    stmt = get_statement(INSERT_DAT);

    stmt->set_string("name", dat.name);
    stmt->set_string("version", dat.version);

    if (stmt->step()) {
        return stmt->get_rowid();
    }
    else {
        throw Exception("can't insert dat"); // TODO: details
    }
}


void StatusDB::insert_game(int64_t run_id, const StatusDB::Game& game) {
    auto stmt = get_statement(INSERT_GAME);

    stmt->set_int64("run_id", run_id);
    stmt->set_string("name", game.name);
    stmt->set_int("status", static_cast<int>(game.status));

    if (!stmt->step()) {
        throw Exception("can't insert game");
    }
}

int64_t StatusDB::insert_run(time_t date) {
    auto stmt = get_statement(INSERT_RUN);

    stmt->set_int64("date", static_cast<int64_t>(date));

    if (stmt->step()) {
        return stmt->get_rowid();
    }
    else {
        throw Exception("can't insert run");
    }
}
