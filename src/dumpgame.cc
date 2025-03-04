/*
  dumpgame.cc -- print info about game (from data base)
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

#include "Dumpgame.h"

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

std::vector<Commandline::Option> dumpgame_options = {
    Commandline::Option("brief", 'b', "brief listing (omit ROM details)"),
    Commandline::Option("checksum", 'c', "find games containing ROMs or disks with given checksums"),
    Commandline::Option("dats", "list dats"),
    Commandline::Option("detector", "print detector"),
    Commandline::Option("disks", "list disks"),
    Commandline::Option("games", "list games"),
    Commandline::Option("hash-types", "show which hash types are used"),
    Commandline::Option("mia", "list games with ROMs marked as mia"),
    Commandline::Option("summary", "print summary of ROM set")};

std::unordered_set<std::string> dumpgame_used_variables = {"rom_db"};

Dumpgame::Dumpgame() : Command("dumpgame", "[game|checksum ...]", dumpgame_options, dumpgame_used_variables), brief_mode(false), find_checksum(false), first(true) {}

static const char *where_name[] = {"game", "cloneof", "grand-cloneof"};


std::string Dumpgame::format_checksums(const Hashes *hashes) {
    std::string result;
    for (int i = 1; i <= Hashes::TYPE_MAX; i <<= 1) {
        if (hashes->has_type(i)) {
            result += " " + Hashes::type_name(i) + " " + hashes->to_string(i);
        }
    }
    return result;
}


void Dumpgame::print_diskline(Rom *disk) {
    auto message = "\t\tdisk " + disk->name + " " + format_checksums(&disk->hashes) + " status " + disk->status_name(true) + " in " + where_name[disk->where];
    if (!disk->merge.empty() && disk->name != disk->merge) {
        message += " (" + disk->merge + ")";
    }
    output.message(message);
}


void Dumpgame::print_romline(Rom *rom) {
    auto message = "\t\tfile " + rom->name + "  size ";
    if (rom->is_size_known()) {
        message += std::to_string(rom->hashes.size);
    }
    else {
        message += "unknown";
    }
    message += format_checksums(&rom->hashes);
    message += " status " + rom->status_name(true);
    if (rom->mia) {
        message += " mia";
    }
    message += std::string(" in ") + where_name[rom->where];
    if (!rom->merge.empty() && rom->name != rom->merge) {
        message += " (" + rom->merge + ")";
    }
    output.message(message);
}


void Dumpgame::print_match(const GamePtr& game, filetype_t ft, size_t i) {
    static std::string name;

    if (name.empty() || game->name != name) {
        output.message("In game " + game->name + ":");
    }

    if (ft == TYPE_DISK) {
        print_diskline(&game->files[ft][i]);
    }
    else {
        print_romline(&game->files[ft][i]);
    }
}


void Dumpgame::print_matches(Hashes *hash) {
    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        auto filetype = static_cast<filetype_t>(ft);
        auto matches = db->read_file_by_hash(filetype, *hash);

        for (const auto &match : matches) {
            if ((match.rom.hashes.get_types() & hash->get_types()) != hash->get_types()) {
                continue;
            }
            auto game = db->read_game(match.game_name);
            if (!game) {
                output.error("db error: %s not found, though in hash index", match.game_name.c_str());
                /* TODO: remember error */
                continue;
            }

            print_match(game, filetype, match.index);
        }
    }
}


int main(int argc, char **argv) {
    auto command = Dumpgame();

    return command.run(argc, argv);
}


void Dumpgame::global_setup(const ParsedCommandline &commandline) {
    for (const auto &option : commandline.options) {
        if (option.name == "brief") {
            brief_mode = true;
        }
        else if (option.name == "checksum") {
            find_checksum = true;
        }
        else if (option.name == "dats") {
            specials.insert(DATS);
        }
        else if (option.name == "detector") {
            specials.insert(DETECTOR);
        }
        else if (option.name == "disks") {
            specials.insert(DISKS);
        }
        else if (option.name == "hash-types") {
            specials.insert(HASH_TYPES);
        }
        else if (option.name == "games") {
            specials.insert(GAMES);
        }
        else if (option.name == "mia") {
            specials.insert(MIA);
        }
        else if (option.name == "summary") {
            specials.insert(SUMMARY);
        }
    }

    arguments = commandline.arguments;
    found.resize(arguments.size(), false);
}


bool Dumpgame::execute(const std::vector<std::string> &arguments_) {
    try {
        db = std::make_unique<RomDB>(configuration.rom_db, DBH_READ);
    } catch (std::exception &e) {
        // TODO: catch exception for unsupported database version and report differently
        output.error("can't open database '%s': %s", configuration.rom_db.c_str(), strerror(errno));
        return false;
    }
    output.set_error_database(db.get());

    std::vector<std::string> list;

    try {
        list = db->read_list(DBH_KEY_LIST_GAME);
    } catch (Exception &e) {
        output.error("list of games not found in database '%s': %s", configuration.rom_db.c_str(), e.what());
        return false;
    }
    std::sort(list.begin(), list.end());

    for (auto key : specials) {
        switch (key) {
            case DATS:
                dump_dats();
                break;

            case DETECTOR:
                dump_detector();
                break;

            case DISKS:
                dump_list(DBH_KEY_LIST_DISK);
                break;

            case GAMES:
                dump_list(DBH_KEY_LIST_GAME);
                break;

            case HASH_TYPES:
                dump_hash_types();
                break;

            case MIA:
                dump_list(DBH_KEY_LIST_MIA);
                break;

            case SUMMARY:
                dump_stats();
                break;
        }
    }


    /* find matches for ROMs */
    if (find_checksum) {
        Hashes match;

        for (const auto &argument : arguments) {
            if (match.set_from_string(argument) == -1) {
                output.error("error parsing checksum '%s'", argument.c_str());
                continue;
            }

            print_matches(&match);
        }
        return true;
    }

    size_t index = 0;
    for (const auto &argument : arguments) {
        if (!is_pattern(argument)) {
            if (first) {
                first = false;
            }
            else {
                output.message(static_cast<std::string>(""));
            }
            if (std::binary_search(list.begin(), list.end(), argument)) {
                found[index] = true;
                dump_game(argument);
            }
        }
        else {
            for (const auto &name : list) {
                if (fnmatch(argument.c_str(), name.c_str(), 0) == 0) {
                    if (first) {
                        first = false;
                    }
                    else {
                        output.message(static_cast<std::string>(""));
                    }
                    dump_game(name);
                    found[index] = true;
                }
            }
        }
        index += 1;
    }

    db = nullptr;

    return true;
}


bool Dumpgame::global_cleanup() {
    auto ok = true;

    if (!find_checksum) {
        size_t index = 0;
        for (const std::string &argument : arguments) {
            if (!found[index]) {
                if (is_pattern(arguments[index])) {
                    output.error("no game matching '%s' found", argument.c_str());
                }
                else {
                    output.error("game '%s' not found", argument.c_str());
                }
                ok = false;
            }
            index += 1;
        }
    }

    return ok;
}


void Dumpgame::print_clones(const GamePtr& game) {
    auto clones = db->get_clones(game->name);

    if (clones.empty()) {
        return;
    }

    size_t i;
    std::string message;
    for (i = 0; i < clones.size(); i++) {
        if (i == 0) {
            message = "Clones:";
        }
        if (i % 6 == 0) {
            message += "\t\t";
        }
        message += pad_string(clones[i], 8) + " ";
        if (i % 6 == 5) {
            output.message(message);
            message = "";
        }
    }
    if (!message.empty()) {
        output.message(message);
    }
}


bool Dumpgame::dump_game(const std::string &name) const {
    GamePtr game;

    auto dat = db->read_dat();

    if (dat.empty()) {
        output.error("cannot read dat info");
        return false;
    }

    if ((game = db->read_game(name)) == nullptr) {
        output.error("game unknown (or database error): '%s'", name.c_str());
        return false;
    }

    output.message("Name:\t\t" + game->name);
    if (dat.size() > 1) {
        output.message("Source:\t\t" + format_dat(dat[game->dat_no]));
    }
    if (!game->description.empty()) {
        output.message("Description:\t" + game->description);
    }

    if (!brief_mode) {
        if (!game->cloneof[0].empty()) {
            output.message("Cloneof:\t" + game->cloneof[0]);
        }
        if (!game->cloneof[1].empty()) {
            output.message("Grand-Cloneof:\t" + game->cloneof[1]);
        }

        print_clones(game);

        if (!game->files[TYPE_ROM].empty()) {
            output.message("ROMs:");
            for (auto &file : game->files[TYPE_ROM]) {
                print_romline(&file);
            }
        }

        if (!game->files[TYPE_DISK].empty()) {
            output.message("Disks:");
            for (auto &file : game->files[TYPE_DISK]) {
                print_diskline(&file);
            }
        }
    }

    return true;
}


void Dumpgame::dump_hash_types() {
    output.message("roms: " + format_hash_types(db->hashtypes(TYPE_ROM)));
    output.message("disks: " + format_hash_types(db->hashtypes(TYPE_DISK)));
}


bool Dumpgame::dump_list(int type) {
    try {
        auto list = db->read_list(static_cast<enum dbh_list>(type));
        sort_strings_case_insensitive(list);

        for (const auto &name : list) {
            output.message(name);
        }
    } catch (Exception &e) {
        output.error_database("db error reading list: %s", e.what());
        return false;
    }

    return true;
}


bool Dumpgame::dump_dats() {
    auto dat = db->read_dat();

    if (dat.empty()) {
        output.error("db error reading dats");
        return false;
    }

    for (size_t i = 0; i < dat.size(); i++) {
        std::string message;
        if (dat.size() > 1) {
            message += pad_string_left(std::to_string(i), 2) + ": ";
        }
        message += format_dat(dat[i]);
        output.message(message);
    }

    return true;
}


void Dumpgame::dump_detector() {
    DetectorPtr detector;

    if (!db->detectors.empty()) {
        detector = db->detectors.begin()->second;
        std::string message;
        message += detector->name;
        if (!detector->version.empty()) {
            message += " (" + detector->version + ")";
        }
        output.message(message);
    }
}


void Dumpgame::dump_stats() {
    auto stats = db->get_stats();

    stats.print(stdout, true);
}


std::string Dumpgame::format_dat(const DatEntry &de) {
    return ((de.name.empty() ? "unknown" : de.name) + " (" + (de.version.empty() ? "unknown" : de.version) + ")");
}


std::string Dumpgame::format_hash_types(int ht) {
    std::string message;
    if (ht & Hashes::TYPE_CRC) {
        message = "crc";
    }
    if (ht & Hashes::TYPE_MD5) {
        if (message.length() > 0) {
            message += ", ";
        }
        message += "md5";
    }
    if (ht & Hashes::TYPE_SHA1) {
        if (message.length() > 0) {
            message += ", ";
        }
        message += "sha1";
    }
    return message;
}


bool Dumpgame::is_pattern(const std::string &string) {
    return strcspn(string.c_str(), "*?[]{}") != string.length();
}
