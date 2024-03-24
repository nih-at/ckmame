/*
  ckmame.c -- main routine for ckmame
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

#include "CkMame.h"

#include <algorithm>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <string>

#include "compat.h"
#include "config.h"

#include "CkmameCache.h"
#include "CkmameDB.h"
#include "Commandline.h"
#include "Configuration.h"
#include "Exception.h"
#include "Fixdat.h"
#include "ProgramName.h"
#include "Progress.h"
#include "RomDB.h"
#include "Stats.h"
#include "Tree.h"
#include "check_util.h"
#include "cleanup.h"
#include "globals.h"
#include "superfluous.h"
#include "update_romdb.h"
#include "util.h"


/* to identify roms directory uniquely */
std::filesystem::path rom_dir_normalized;


std::vector<Commandline::Option> ckmame_options = {
    Commandline::Option("fix", 'F', "fix ROM set"),
    Commandline::Option("game-list", 'T', "file", "read games to check from file"),
    Commandline::Option("only-if-database-updated", 'U', "if dats didn't change, exit; otherwise update database and run"),
    Commandline::Option("trace", "trace actions, useful for profiling")
};

std::unordered_set<std::string> ckmame_used_variables = {
    "complete_games_only",
    "complete_list",
    "create_fixdat",
    "delete_unknown_pattern",
    "extra_directories",
    "fixdat_directory",
    "keep_old_duplicate",
    "missing_list",
    "move_from_extra",
    "old_db",
    "report_changes",
    "report_correct",
    "report_detailed",
    "report_fixable",
    "report_missing",
    "report_no_good_dump",
    "report_summary",
    "rom_db",
    "rom_directory",
    "roms_zipped",
    "saved_directory",
    "unknown_directory",
    "update_database",
    "use_torrentzip",
    "verbose"
};

static bool contains_romdir(const std::string &ame);
static std::string diff_stat(const std::vector<std::string>& old_lines, const std::vector<std::string>& new_lines);

int main(int argc, char **argv) {
    auto command = CkMame();

    return command.run(argc, argv);
}

CkMame::CkMame() : Command("ckmame", "[game ...]", ckmame_options, ckmame_used_variables), only_if_updated(false) {
}

void CkMame::global_setup(const ParsedCommandline &commandline) {
    for (const auto &option : commandline.options) {
        if (option.name == "fix") {
            configuration.fix_romset = true;
        }
        else if (option.name == "game-list") {
            game_list = option.argument;
        }
        else if (option.name == "only-if-database-updated") {
            only_if_updated = true;
        }
        else if (option.name == "trace") {
            Progress::trace = true;
        }
    }

    if (!configuration.fix_romset) {
        Archive::read_only_mode = true;
    }
}

bool CkMame::execute(const std::vector<std::string> &arguments) {
    int found;
    auto checking_all_games = false;

    if (only_if_updated) {
        configuration.update_database = true;
    }

    if (configuration.update_database) {
        try {
            auto updated = update_romdb();
            if (!updated && only_if_updated) {
                return true;
            }
        }
        catch (std::exception &e) {
            output.error("can't update database '%s': %s", configuration.rom_db.c_str(), e.what());
            return false;
        }
    }

    try {
        db = std::make_unique<RomDB>(configuration.rom_db, DBH_READ);
    } catch (std::exception &e) {
        output.error("can't open database '%s': %s", configuration.rom_db.c_str(), e.what());
        return false;
    }
    try {
        old_db = std::make_unique<RomDB>(configuration.old_db, DBH_READ);
    } catch (std::exception &e) {
        /* TODO: check for errors other than ENOENT */
    }

    if (!configuration.roms_zipped && db->has_disks() == 1) {
        fprintf(stderr, "%s: unzipped mode is not supported for ROM sets with disks\n", ProgramName::get().c_str());
        return false;
    }

    ensure_dir(configuration.rom_directory, false);
    std::error_code ec;
    rom_dir_normalized = std::filesystem::relative(configuration.rom_directory, "/", ec);
    if (ec || rom_dir_normalized.empty()) {
        /* TODO: treat as warning only? (this exits if any ancestor directory is unreadable) */
        output.error_system("can't normalize directory '%s'", configuration.rom_directory.c_str());
        return false;
    }

    ckmame_cache = std::make_shared<CkmameCache>();

    try {
        ckmame_cache->register_directory(configuration.rom_directory, FILE_ROMSET);
        ckmame_cache->register_directory(configuration.saved_directory, FILE_NEEDED);
        ckmame_cache->register_directory(configuration.unknown_directory, FILE_EXTRA);
        for (const auto &name : configuration.extra_directories) {
            if (contains_romdir(name)) {
                /* TODO: improve error message: also if extra is in ROM directory. */
                output.error("current ROM directory '%s' is in extra directory '%s'", configuration.rom_directory.c_str(), name.c_str());
                return false;
            }
            ckmame_cache->register_directory(name, FILE_EXTRA);
        }
    } catch (Exception &exception) {
        // TODO: handle error
        fprintf(stderr, "%s: %s\n", ProgramName::get().c_str(), exception.what());
        return false;
    }

    if (configuration.create_fixdat) {
        Fixdat::begin();
    }

    /* build tree of games to check */
    std::vector<std::string> list;

    try {
        list = db->read_list(DBH_KEY_LIST_GAME);
    } catch (Exception &e) {
        output.error("list of games not found in database '%s': %s", configuration.rom_db.c_str(), e.what());
        return false;
    }
    std::sort(list.begin(), list.end());

    if (!game_list.empty()) {
        char b[8192];

        output.set_error_file(game_list);

        auto f = make_shared_file(game_list, "r");
        if (!f) {
            output.archive_error_system("cannot open game list");
            exit(1);
        }

        while (fgets(b, sizeof(b), f.get())) {
            if (b[strlen(b) - 1] == '\n')
                b[strlen(b) - 1] = '\0';
            else {
                output.archive_error("overly long line ignored");
                continue;
            }

            if (std::binary_search(list.begin(), list.end(), b)) {
                check_tree.add(b);
            }
            else {
                output.error("game '%s' unknown", b);
            }
        }
    }
    else if (arguments.empty()) {
        checking_all_games = true;
        for (const auto &name : list) {
            check_tree.add(name);
        }
    }
    else {
        for (const auto &argument : arguments) {
            if (strcspn(argument.c_str(), "*?[]{}") == argument.size()) {
                if (std::binary_search(list.begin(), list.end(), argument)) {
                    check_tree.add(argument);
                }
                else {
                    output.error("game '%s' unknown", argument.c_str());
                }
            }
            else {
                found = 0;
                for (const auto &j : list) {
                    if (fnmatch(argument.c_str(), j.c_str(), 0) == 0) {
                        check_tree.add(j);
                        found = 1;
                    }
                }
                if (!found)
                    output.error("no game matching '%s' found", argument.c_str());
            }
        }
    }

    if (!ckmame_cache->superfluous_delete_list) {
        ckmame_cache->superfluous_delete_list = std::make_shared<DeleteList>();
    }
    ckmame_cache->superfluous_delete_list->add_directory(configuration.rom_directory, true);

    if (configuration.fix_romset) {
        ckmame_cache->ensure_extra_maps();
    }

    Progress::enable();

    check_tree.traverse();
    check_tree.traverse(); /* handle rechecks */

    if (configuration.fix_romset) {
        if (!ckmame_cache->needed_delete_list) {
            ckmame_cache->needed_delete_list = std::make_shared<DeleteList>();
        }
        if (ckmame_cache->needed_delete_list->archives.empty()) {
            ckmame_cache->needed_delete_list->add_directory(configuration.saved_directory, false);
        }
        cleanup_list(ckmame_cache->superfluous_delete_list, CLEANUP_NEEDED | CLEANUP_UNKNOWN, FILE_SUPERFLUOUS);
        cleanup_list(ckmame_cache->needed_delete_list, CLEANUP_UNKNOWN, FILE_NEEDED);
    }

    if (configuration.create_fixdat) {
        Fixdat::end();
    }

    if (configuration.fix_romset) {
        cleanup_list(ckmame_cache->extra_delete_list, 0, FILE_EXTRA);
    }

    if (arguments.empty()) {
        print_superfluous(ckmame_cache->superfluous_delete_list);
    }

    if (configuration.report_summary) {
        ckmame_cache->stats.print(stdout, false);
    }

    if (checking_all_games && (!configuration.complete_list.empty() || !configuration.missing_list.empty())) {
        std::vector<std::string> new_complete;
        std::vector<std::string> new_missing;
        std::vector<std::string> previous_complete;
        std::vector<std::string> previous_missing;

        if (!configuration.complete_list.empty() && std::filesystem::exists(configuration.complete_list)) {
            previous_complete = slurp_lines(configuration.complete_list);
            std::sort(previous_complete.begin(), previous_complete.end());
        }
        if (!configuration.missing_list.empty() && std::filesystem::exists(configuration.missing_list)) {
            previous_missing = slurp_lines(configuration.missing_list);
            std::sort(previous_missing.begin(), previous_missing.end());
        }

        for (const auto& name : list) {
            if (ckmame_cache->complete_games.find(name) != ckmame_cache->complete_games.end()) {
                if (!configuration.complete_list.empty()) {
                    new_complete.emplace_back(name);
                }
            }
            else {
                if (!configuration.missing_list.empty()) {
                    new_missing.emplace_back(name);
                }
            }
        }

        if (!configuration.complete_list.empty()) {
            if (new_complete.empty()) {
                std::filesystem::remove(configuration.complete_list, ec);
                // TODO: throw all errors except ENOENT, if anyone can figure out how that's done in C++
            }
            else {
                write_lines(configuration.complete_list, new_complete);
            }
        }
        if (!configuration.missing_list.empty()) {
            if (new_missing.empty()) {
                std::filesystem::remove(configuration.missing_list, ec);
                // TODO: throw all errors except ENOENT, if anyone can figure out how that's done in C++
            }
            else {
                write_lines(configuration.missing_list, new_missing);
            }
        }
        if (configuration.report_changes) {
            auto stats_complete = diff_stat(previous_complete, new_complete);
            auto stats_missing = diff_stat(previous_missing, new_missing);
            auto stats = std::string();

            if (!stats_complete.empty()) {
                stats = "complete: " + stats_complete;
            }
            if (!stats_missing.empty()) {
                if (!stats.empty()) {
                    stats += ", ";
                }
                stats += "missing: " + stats_missing;
            }
            if (!stats.empty()) {
                output.message(stats);
            }
        }
    }

    if (configuration.fix_romset) {
        std::error_code ec;
        std::filesystem::remove(configuration.saved_directory, ec);
        // Since we create saved/$set by default, remove saved. This is not entirely clean, since we also do this in the non-default case.
        std::filesystem::remove(std::filesystem::path(configuration.saved_directory).parent_path(), ec);
    }

    return true;
}




bool CkMame::cleanup() {
    db = nullptr;
    old_db = nullptr;
    check_tree.clear();
    ckmame_cache = nullptr;
    ArchiveContents::clear_cache();

    return true;
}


static bool
contains_romdir(const std::string &name) {
    std::error_code ec;
    auto normalized = std::filesystem::relative(name, "/", ec);
    if (ec || normalized.empty()) {
	return false;
    }

    auto it_extra = normalized.begin();
    auto it_romdir = rom_dir_normalized.begin();

    while (it_romdir != rom_dir_normalized.end() && it_extra != normalized.end()) {
	if (*it_extra != *it_romdir) {
	    return false;
	}
	it_extra++;
	it_romdir++;
    }

    return it_extra == normalized.end();
}

static std::string diff_stat(const std::vector<std::string>& old_lines, const std::vector<std::string>& new_lines) {
    size_t added, removed;
    diff_lines(old_lines, new_lines, added, removed);

    if (added == 0 && removed == 0) {
        return "";
    }
    auto s = std::to_string(old_lines.size()) + " -> " + std::to_string(new_lines.size()) + " (";
    if (added > 0) {
        s += "+" + std::to_string(added);
        if (removed > 0) {
            s += "/";
        }
    }
    if (removed > 0) {
        s += "-" + std::to_string(removed);
    }
    s += ")";

    return s;
}
