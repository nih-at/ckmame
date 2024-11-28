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

std::shared_ptr<StatusDB> status_db;

const DB::DBFormat StatusDB::format = {0x3,
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
    run_id integer not null,\n\
    dat_id integer not null,\n\
    name text not null,\n\
    checksum binary not null,\n\
    status integer not null\n\
);",
                                       {}};

std::unordered_map<int, std::string> StatusDB::queries = {
    {CLEANUP_DAT, "delete from dat where dat_id not in (select distinct(dat_id) from game)"},
    {CLEANUP_GAME, "delete from game where run_id not in (select run_id from run)"},
    {DELETE_RUN_BOTH, "delete from run where date < :date and run_id not in (select run_id from run order by date "
                      "desc limit :count)"},
    {DELETE_RUN_COUNT,
     "delete from run where run_id not in (select run_id from run order by date desc limit :count)"},
    {DELETE_RUN_DATE, "delete from run where date < :date"},
    {FIND_DAT, "select dat_id from dat where name = :name and version = :version"},
    {INSERT_DAT, "insert into dat (name, version) values (:name, :version)"},
    {INSERT_GAME,
     "insert into game (run_id, dat_id, name, checksum, status) values (:run_id, :dat_id, :name, :checksum, :status)"},
    {INSERT_RUN, "insert into run (date) values (:date)"},
    {LATEST_RUN_ID, "select run_id from run order by date desc limit 2"},
    {LIST_RUNS, "select run_id, date from run order by date asc"},
    {QUERY_GAME, "select dat_id, name, checksum, status from game where run_id = :run_id"},
    {QUERY_GAME_BY_STATUS, "select name from game where run_id = :run_id and status = :status order by name"},
    {QUERY_GAME_BY_STATUS2, "select name from game where run_id = :run_id and status in (:status1, :status2) order by name"},
    {QUERY_GAME_BY_STATUS3, "select name from game where run_id = :run_id and status in (:status1, :status2, :status3) order by name"},
    {QUERY_GAME_BY_STATUS4, "select name from game where run_id = :run_id and status in (:status1, :status2, :status3, :status4) order by name"},
    {QUERY_GAME_BY_STATUS6, "select name from game where run_id = :run_id and status in (:status1, :status2, :status3, :status4, :status5, :status6) order by name"},
    {QUERY_GAME_STATI, "select name, status from game where run_id = :run order by name"},
    {QUERY_RUN_STATUS_COUNTS,
     "select status, count(*) as status_count from game where run_id = :run_id group by status"},
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

void StatusDB::delete_runs(std::optional<int> days, std::optional<int> count) {
    DBStatement* stmt{};

    if (days) {
        if (count) {
            stmt = get_statement(DELETE_RUN_BOTH);
        }

        stmt = get_statement(DELETE_RUN_DATE);
    }
    else if (count) {
        stmt = get_statement(DELETE_RUN_COUNT);
    }
    else {
        return;
    }

    if (days) {
        stmt->set_int64("oldest", static_cast<int64_t>(time(nullptr) - *days * 24 * 60 * 60));
    }
    if (count) {
        stmt->set_int("count", *count);
    }

    stmt->execute();

    stmt = get_statement(CLEANUP_GAME);
    stmt->execute();

    stmt = get_statement(CLEANUP_DAT);
    stmt->execute();
}

std::vector<StatusDB::GameInfo> StatusDB::get_games(int64_t run_id) {
    auto stmt = get_statement(QUERY_GAME);

    stmt->set_int64("run_id", run_id);

    std::vector<GameInfo> games;

    while (stmt->step()) {
        GameInfo game;

        game.dat_id = stmt->get_int("dat_id");
        game.name = stmt->get_string("name");
        game.checksum = stmt->get_blob("checksum");
        game.status = static_cast<GameStatus>(stmt->get_int("status"));

        games.push_back(game);
    }

    return games;
}

std::optional<int> StatusDB::latest_run_id(bool second) {
    auto stmt = get_statement(LATEST_RUN_ID);

    if (!stmt->step()) {
        return {};
    }
    if (second && !stmt->step()) {
        return {};
    }

    return stmt->get_int("run_id");
}



std::vector<StatusDB::Run> StatusDB::list_runs() {
    auto stmt = get_statement(LIST_RUNS);

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

std::vector<std::string> StatusDB::get_games_by_status(int64_t run_id, GameStatus status1, GameStatus status2) {
    auto stmt = get_statement(QUERY_GAME_BY_STATUS2);

    stmt->set_int64("run_id", run_id);
    stmt->set_int("status1", status1);
    stmt->set_int("status2", status2);

    std::vector<std::string> games;

    while (stmt->step()) {
        games.push_back(stmt->get_string("name"));
    }

    return games;
}

std::vector<std::string> StatusDB::get_games_by_status(int64_t run_id, GameStatus status1, GameStatus status2, GameStatus status3) {
    auto stmt = get_statement(QUERY_GAME_BY_STATUS4);

    stmt->set_int64("run_id", run_id);
    stmt->set_int("status1", status1);
    stmt->set_int("status2", status2);
    stmt->set_int("status3", status3);

    std::vector<std::string> games;

    while (stmt->step()) {
        games.push_back(stmt->get_string("name"));
    }

    return games;
}

std::vector<std::string> StatusDB::get_games_by_status(int64_t run_id, GameStatus status1, GameStatus status2, GameStatus status3, GameStatus status4) {
    auto stmt = get_statement(QUERY_GAME_BY_STATUS4);

    stmt->set_int64("run_id", run_id);
    stmt->set_int("status1", status1);
    stmt->set_int("status2", status2);
    stmt->set_int("status3", status3);
    stmt->set_int("status4", status4);

    std::vector<std::string> games;

    while (stmt->step()) {
        games.push_back(stmt->get_string("name"));
    }

    return games;
}

std::vector<std::string> StatusDB::get_games_by_status(int64_t run_id, GameStatus status1, GameStatus status2, GameStatus status3, GameStatus status4, GameStatus status5, GameStatus status6) {
    auto stmt = get_statement(QUERY_GAME_BY_STATUS6);

    stmt->set_int64("run_id", run_id);
    stmt->set_int("status1", status1);
    stmt->set_int("status2", status2);
    stmt->set_int("status3", status3);
    stmt->set_int("status4", status4);
    stmt->set_int("status4", status5);
    stmt->set_int("status4", status6);

    std::vector<std::string> games;

    while (stmt->step()) {
        games.push_back(stmt->get_string("name"));
    }

    return games;
}

std::unordered_map<GameStatus, std::vector<std::string>> StatusDB::get_run_status_names(int64_t run_id) {
    auto stmt = get_statement(QUERY_GAME_STATI);

    stmt->set_int64("run_id", run_id);

    std::unordered_map<GameStatus, std::vector<std::string>> names;

    while (stmt->step()) {
        auto status = static_cast<GameStatus>(stmt->get_int("status"));

        if (names.find(status) == names.end()) {
            names[status] = std::vector<std::string>();
        }

        names[status].push_back(stmt->get_string("name"));
    }

    return names;
}

int64_t StatusDB::find_dat(const DatEntry& dat) {
    auto stmt = get_statement(FIND_DAT);
    stmt->set_string("name", dat.name);
    stmt->set_string("version", dat.version);

    while (stmt->step()) {
        return stmt->get_int("dat_id");
    }

    return -1;
}


std::unordered_map<GameStatus, uint64_t> StatusDB::get_run_status_counts(int64_t run_id) {
    auto stmt = get_statement(QUERY_RUN_STATUS_COUNTS);

    stmt->set_int64("run_id", run_id);

    std::unordered_map<GameStatus, uint64_t> counts;

    while (stmt->step()) {
        auto status = static_cast<GameStatus>(stmt->get_int("status"));

        if (counts.find(status) == counts.end()) {
            counts[status] = 0;
        }

        counts[status] += stmt->get_int("status_count");
    }

    return counts;
}

int64_t StatusDB::insert_dat(const DatEntry& dat) {
    auto stmt = get_statement(INSERT_DAT);

    stmt->set_string("name", dat.name);
    stmt->set_string("version", dat.version);

    stmt->execute();
    return stmt->get_rowid();
}


void StatusDB::insert_game(int64_t run_id, const Game& game, int64_t dat_id, GameStatus status) {
    auto stmt = get_statement(INSERT_GAME);

    if (status == GS_FIXABLE) {
        if (game.is_mia()) {
            status = GS_CORRECT_MIA;
        }
        else {
            status = GS_CORRECT;
        }
    }

    std::vector<uint8_t> checksum;

    compute_combined_checksum(game, checksum);

    stmt->set_int64("run_id", run_id);
    stmt->set_uint64("dat_id", dat_id);
    stmt->set_string("name", game.name);
    stmt->set_blob("checksum", checksum);
    stmt->set_int("status", static_cast<int>(status));

    stmt->execute();
}

int64_t StatusDB::insert_run(time_t date) {
    auto stmt = get_statement(INSERT_RUN);

    stmt->set_int64("date", static_cast<int64_t>(date));

    stmt->execute();
    return stmt->get_rowid();
}

void StatusDB::compute_combined_checksum(const Game& game, std::vector<uint8_t>& checksum) {
    if (game.files[TYPE_ROM].size() == 1 && game.files[TYPE_DISK].empty()) {
        checksum = game.files[TYPE_ROM][0].hashes.get_best();
    }
    else if (game.files[TYPE_ROM].empty() && game.files[TYPE_DISK].size() == 1) {
        checksum = game.files[TYPE_DISK][0].hashes.get_best();
    }
    else {
        checksum.clear();

        for (int type = TYPE_ROM; type < TYPE_MAX; type += 1) {
            for (const auto& file : game.files[type]) {
                auto file_checksum = file.hashes.get_best();
                if (checksum.size() < file_checksum.size()) {
                    checksum.resize(file_checksum.size());
                }
                for (size_t i = 0; i < file_checksum.size(); i++) {
                    checksum[i] = checksum[i] ^ file_checksum[i];
                }
            }
        }
    }
}
