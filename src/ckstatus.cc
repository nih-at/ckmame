/*
  ckstatus.cc -- print info about last runs (from data base)
  Copyright (C) 1999-2018 Dieter Baron and Thomas Klausner

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

#include "CkStatus.h"

#include <algorithm>
#include <cerrno>
#include <cstring>

#include "compat.h"

#include "Commandline.h"
#include "Exception.h"
#include "RomDB.h"
#include "Stats.h"
#include "globals.h"
#include "util.h"

std::vector<Commandline::Option> ckstatus_options = {
    Commandline::Option("all-missing", "list missing games"),
    Commandline::Option("changes", "list changes between runs"), Commandline::Option("correct", "list correct games"),
    Commandline::Option("correct-mia", "list correct games marked MIA"),
    Commandline::Option("missing", "list missing games not marked MIA"),
    //  Commandline::Option("run", "RUN", "use information from run RUN (default: latest)"),
    Commandline::Option("runs", "list runs")};

std::unordered_set<std::string> ckstatus_used_variables = {"status_db"};

int main(int argc, char** argv) {
    auto command = CkStatus();

    return command.run(argc, argv);
}


CkStatus::CkStatus() : Command("ckstatus", "", ckstatus_options, ckstatus_used_variables) {}

void CkStatus::global_setup(const ParsedCommandline& commandline) {
    for (const auto& option : commandline.options) {
        if (option.name == "all-missing") {
            specials.insert(ALL_MISSING);
        }
        else if (option.name == "changes") {
            specials.insert(CHANGES);
        }
        else if (option.name == "correct") {
            specials.insert(CORRECT);
        }
        else if (option.name == "correct-mia") {
            specials.insert(CORRECT_MIA);
        }
        else if (option.name == "missing") {
            specials.insert(MISSING);
        }
        else if (option.name == "runs") {
            specials.insert(RUNS);
        }
    }
}


bool CkStatus::global_cleanup() { return true; }


bool CkStatus::execute(const std::vector<std::string>& arguments) {
    if (configuration.status_db == "none") {
        throw Exception("No status database configured.");
    }

    try {
        status_db = std::make_shared<StatusDB>(configuration.status_db, DBH_WRITE | DBH_CREATE);
    }
    catch (const std::exception& e) {
        throw Exception("Error opening status database: " + std::string(e.what()));
    }

    for (auto key : specials) {
        switch (key) {
        case ALL_MISSING:
            list_games(GS_MISSING, GS_MISSING_MIA);
            break;

        case CHANGES:
            print_changes();
            break;

        case CORRECT:
            list_games(GS_CORRECT, GS_CORRECT_MIA);
            break;

        case CORRECT_MIA:
            list_games(GS_CORRECT_MIA);
            break;

        case MISSING:
            list_games(GS_MISSING);
            break;
        case RUNS:
            list_runs();
            break;
        }
    }
    return true;
}

int CkStatus::get_run_id() {
    if (!run_id) {
        run_id = status_db->latest_run_id(false);
        if (!run_id) {
            throw Exception("No runs.");
        }
    }
    return *run_id;
}

int CkStatus::get_run_id2() {
    if (!run_id2) {
        run_id2 = status_db->latest_run_id(true);
        if (!run_id2) {
            throw Exception("Only one run.");
        }
    }
    return *run_id2;
}


void CkStatus::list_games(GameStatus status) { list_games(status_db->get_games_by_status(get_run_id(), status)); }

void CkStatus::list_games(GameStatus status1, GameStatus status2) {
    list_games(status_db->get_games_by_status(get_run_id(), status1, status2));
}

void CkStatus::list_games(const std::vector<std::string>& games) {
    for (const auto& game : games) {
        output.message(game);
    }
}


void CkStatus::list_runs() {
    auto runs = status_db->list_runs();

    for (const auto& run : runs) {
        char date_string[20];
        strftime(date_string, sizeof(date_string), "%Y-%m-%d %H:%M:%S", localtime(&run.date));
        output.message("%" PRId64 ": %s", run.run_id, date_string);
    }
}

void CkStatus::print_changes() {
    auto diffs = RunDiff(get_run_id(), get_run_id2());

    diffs.compute();
}

void CkStatus::RunDiff::compute() {
    insert_run(false);
    insert_run(true);

    std::sort(games.begin(), games.end());

    for (const auto& game : games) {
        auto diff = diffs.at(game.checksum);
        if (diff.new_info) {
            if (is_complete(diff.new_info->status)) {
                if (!diff.old_info || !is_complete(diff.old_info->status)) {
                    output.message("+ %s", game.name.c_str());
                }
            }
        }
        else {
            if (is_complete(diff.old_info->status)) {
                output.message("- %s", game.name.c_str());
            }
        }
    }
}


void CkStatus::RunDiff::insert_run(bool old) {
    auto infos = status_db->get_games(old ? run_id2 : run_id1);

    for (const auto& info : infos) {
        auto it = diffs.find(info.checksum);
        if (it != diffs.end()) {
            if (old) {
                it->second.old_info = info;
            }
            else {
                it->second.new_info = info;
            }
        }
        else {
            games.push_back({info.name, info.checksum});
            auto diff = GameDiff{};
            if (old) {
                diff.old_info = info;
            }
            else {
                diff.new_info = info;
            }
            diffs[info.checksum] = diff;
        }
    }
}
