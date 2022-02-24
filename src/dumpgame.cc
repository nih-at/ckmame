/*
  dumpgame.c -- print info about game (from data base)
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
#include <cinttypes>
#include <cstring>

#include "compat.h"
#include "config.h"

#include "Commandline.h"
#include "error.h"
#include "Exception.h"
#include "globals.h"
#include "RomDB.h"
#include "Stats.h"
#include "util.h"


static int dump_game(const std::string &, int);
static int dump_hashtypes(int);
static int dump_list(int);
static int dump_dat(int);
static int dump_special(const char *);
static int dump_stats(int);
static void print_clones(GamePtr game);
static void print_dat(const DatEntry &de);
static void print_hashtypes(int);

std::vector<Commandline::Option> dumpgame_options = {
    Commandline::Option("brief", 'b', "brief listing (omit ROM details)"),
    Commandline::Option("checksum", 'c', "find games containing ROMs or disks with given checksums"),
};

std::unordered_set<std::string> dumpgame_used_variables = {
    "rom_db"
};

Dumpgame::Dumpgame() : Command("dumpgame", "[game|checksum ...]", dumpgame_options, dumpgame_used_variables), brief_mode(false), find_checksum(false), first(true) {

}

static const char *where_name[] = {"game", "cloneof", "grand-cloneof"};

static void print_checksums(const Hashes *hashes) {
    for (int i = 1; i <= Hashes::TYPE_MAX; i <<= 1) {
	if (hashes->has_type(i)) {
            printf(" %s %s", Hashes::type_name(i).c_str(), hashes->to_string(i).c_str());
        }
    }
}


static void print_diskline(Rom *disk) {
    printf("\t\tdisk %-12s", disk->name.c_str());
    print_checksums(&disk->hashes);
    printf(" status %s in %s", disk->status_name(true).c_str(), where_name[disk->where]);
    if (!disk->merge.empty() && disk->name != disk->merge) {
        printf(" (%s)", disk->merge.c_str());
    }
    putc('\n', stdout);
}


static void print_footer(int matches, Hashes *hash) {
    printf("%d match%s found for checksum", matches, matches == 1 ? "" : "es");
    print_checksums(hash);
    putc('\n', stdout);
}


static void print_romline(Rom *rom) {
    printf("\t\tfile %-12s  size ", rom->name.c_str());
    if (rom->is_size_known()) {
	printf("%7" PRIu64, rom->hashes.size);
    }
    else {
	printf("unknown");
    }
    print_checksums(&rom->hashes);
    printf(" status %s in %s", rom->status_name(true).c_str(), where_name[rom->where]);
    if (!rom->merge.empty() && rom->name != rom->merge) {
	printf(" (%s)", rom->merge.c_str());
    }
    putc('\n', stdout);
}


static void print_match(GamePtr game, filetype_t ft, size_t i) {
    static std::string name;

    if (name.empty() || game->name != name) {
	name = game->name;
	printf("In game %s:\n", name.c_str());
    }

    if (ft == TYPE_DISK) {
	print_diskline(&game->files[ft][i]);
    }
    else {
	print_romline(&game->files[ft][i]);
    }
}


static void print_matches(Hashes *hash) {
    int matches_count = 0;

    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
	auto filetype = static_cast<filetype_t>(ft);
	auto matches = db->read_file_by_hash(filetype, *hash);

	for (const auto &match : matches) {
	    if ((match.rom.hashes.get_types() & hash->get_types()) != hash->get_types()) {
		continue;
	    }
	    auto game = db->read_game(match.game_name);
	    if (!game) {
		myerror(ERRDEF, "db error: %s not found, though in hash index", match.game_name.c_str());
		/* TODO: remember error */
		continue;
	    }

	    print_match(game, filetype, match.index);
	    matches_count++;
	}
    }

    print_footer(matches_count, hash);
}


int main(int argc, char **argv) {
    auto command = Dumpgame();

    return command.run(argc, argv);
}


void Dumpgame::setup(const ParsedCommandline &commandline) {
    for (const auto &option : commandline.options) {
	if (option.name == "brief") {
	    brief_mode = true;
	}
	else if (option.name == "checksum") {
	    find_checksum = true;
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
	myerror(0, "can't open database '%s': %s", configuration.rom_db.c_str(), strerror(errno));
	return false;
    }
    seterrdb(db.get());

    std::vector<std::string> list;

    try {
	list = db->read_list(DBH_KEY_LIST_GAME);
    } catch (Exception &e) {
	myerror(ERRDEF, "list of games not found in database '%s': %s", configuration.rom_db.c_str(), e.what());
	return false;
    }
    std::sort(list.begin(), list.end());

    /* find matches for ROMs */
    if (find_checksum) {
	Hashes match;

	for (const auto &argument : arguments) {
	    if (match.set_from_string(argument) == -1) {
		myerror(ERRDEF, "error parsing checksum '%s'", argument.c_str());
		continue;
	    }

	    print_matches(&match);
	}
	return true;
    }

    size_t index = 0;
    for (const auto &argument : arguments) {
	if (!is_pattern(argument)) {
	    if (argument[0] == '/') {
		if (first) {
		    first = false;
		}
		else {
		    putc('\n', stdout);
		}
		dump_special(argument.c_str());
		found[index] = true;
	    }
	    else if (std::binary_search(list.begin(), list.end(), argument)) {
		if (first) {
		    first = false;
		}
		else {
		    putc('\n', stdout);
		}
		found[index] = true;
		dump_game(argument, brief_mode);
	    }
	}
	else {
	    for (const auto &name : list) {
		if (fnmatch(argument.c_str(), name.c_str(), 0) == 0) {
		    if (first) {
			first = false;
		    }
		    else {
			putc('\n', stdout);
		    }
		    dump_game(name, brief_mode);
		    found[index] = true;
		}
	    }
	}
	index += 1;
    }

    db = nullptr;

    return true;
}


bool Dumpgame::cleanup() {
    auto ok = true;

    if (!find_checksum) {
	size_t index = 0;
	for (const std::string &argument : arguments) {
	    if (!found[index]) {
		if (is_pattern(arguments[index])) {
		    myerror(ERRDEF, "no game matching '%s' found", argument.c_str());
		}
		else {
		    myerror(ERRDEF, "game '%s' not found", argument.c_str());
		}
		ok = false;
	    }
	    index += 1;
	}
    }

    return ok;
}


static void print_clones(GamePtr game) {
    auto clones = db->get_clones(game->name);
    
    if (clones.empty()) {
        return;
    }
    
    size_t i;
    for (i = 0; i < clones.size(); i++) {
        if (i == 0) {
            printf("Clones:");
        }
        if (i % 6 == 0) {
	    printf("\t\t");
        }
	printf("%-8s ", clones[i].c_str());
        if (i % 6 == 5) {
	    printf("\n");
        }
    }
    if (i % 6 != 0) {
	printf("\n");
    }
}


static int
dump_game(const std::string &name, int brief_mode) {
    GamePtr game;

    auto dat = db->read_dat();

    if (dat.empty()) {
	myerror(ERRDEF, "cannot read dat info");
	return -1;
    }

    if ((game = db->read_game(name)) == nullptr) {
	myerror(ERRDEF, "game unknown (or database error): '%s'", name.c_str());
	return -1;
    }

    /* TODO: use print_* functions */
    printf("Name:\t\t%s\n", game->name.c_str());
    if (dat.size() > 1) {
	printf("Source:\t\t");
        print_dat(dat[game->dat_no]);
	putc('\n', stdout);
    }
    if (!game->description.empty()) {
	printf("Description:\t%s\n", game->description.c_str());
    }

    if (!brief_mode) {
        if (!game->cloneof[0].empty()) {
            printf("Cloneof:\t%s\n", game->cloneof[0].c_str());
        }
        if (!game->cloneof[1].empty()) {
            printf("Grand-Cloneof:\t%s\n", game->cloneof[1].c_str());
        }
        
        print_clones(game);

        if (!game->files[TYPE_ROM].empty()) {
            printf("ROMs:\n");
            for (auto &file : game->files[TYPE_ROM]) {
                print_romline(&file);
            }
        }

        if (!game->files[TYPE_DISK].empty()) {
            printf("Disks:\n");
            for (auto &file : game->files[TYPE_DISK]) {
                print_diskline(&file);
            }
        }

    }

    return 0;
}


/*ARGSUSED1*/
static int
dump_hashtypes(int dummy) {
    printf("roms: ");
    print_hashtypes(db->hashtypes(TYPE_ROM));
    printf("\ndisks: ");
    print_hashtypes(db->hashtypes(TYPE_DISK));
    putc('\n', stdout);

    return 0;
}


static int
dump_list(int type) {
    try {
        auto list = db->read_list(static_cast<enum dbh_list>(type));

        for (const auto &name : list) {
            printf("%s\n", name.c_str());
        }
    }
    catch (Exception &e) {
        myerror(ERRDB, "db error reading list: %s", e.what());
        return -1;
    }

    return 0;
}


/*ARGSUSED1*/
static int
dump_dat(int dummy) {
    auto dat = db->read_dat();
    
    if (dat.empty()) {
	myerror(ERRDEF, "db error reading /dat");
	return -1;
    }

    for (size_t i = 0; i < dat.size(); i++) {
        if (dat.size() > 1) {
            printf("%2zu: ", i);
        }
        print_dat(dat[i]);
	putc('\n', stdout);
    }

    return 0;
}


/*ARGSUSED1*/
static int
dump_detector(int dummy) {
    DetectorPtr detector;

    if (!db->detectors.empty()) {
        detector = db->detectors.begin()->second;
        printf("%s", detector->name.c_str());
        if (!detector->version.empty()) {
            printf(" (%s)", detector->version.c_str());
        }
	printf("\n");
    }

    return 0;
}


static int
dump_special(const char *name) {
    static const struct {
	const char *key;
	int (*f)(int);
	int arg;
    } keys[] = {{"/dat", dump_dat, 0}, {"/detector", dump_detector, 0}, {"/hashtypes", dump_hashtypes, 0}, {"/list", dump_list, DBH_KEY_LIST_GAME}, {"/list/disk", dump_list, DBH_KEY_LIST_DISK}, {"/list/game", dump_list, DBH_KEY_LIST_GAME}, {"/stats", dump_stats, 0}};

    for (const auto &key : keys) {
	if (strcasecmp(name, key.key) == 0)
	    return key.f(key.arg);
    }

    myerror(ERRDEF, "unknown special: '%s'", name);
    return -1;
}


/*ARGSUSED1*/
static int
dump_stats(int dummy) {
    stats = db->get_stats();
    
    stats.print(stdout, true);

    return 0;
}


static void print_dat(const DatEntry &de) {
    printf("%s (%s)", de.name.empty() ? "unknown" : de.name.c_str(), de.version.empty() ? "unknown" : de.version.c_str());
}


#define DO(ht, x, s) (((ht) & (x)) ? printf("%s%s", (first ? first = 0, "" : ", "), (s)) : 0)


static void
print_hashtypes(int ht) {
    int first;

    first = 1;

    DO(ht, Hashes::TYPE_CRC, "crc");
    DO(ht, Hashes::TYPE_MD5, "md5");
    DO(ht, Hashes::TYPE_SHA1, "sha1");
}


bool Dumpgame::is_pattern(const std::string &string) {
    return strcspn(string.c_str(), "*?[]{}") != string.length();
}
