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

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <string>

#include <ctime>

#include "compat.h"
#include "config.h"

#include "check_util.h"
#include "cleanup.h"
#include "CkmameDB.h"
#include "Commandline.h"
#include "Configuration.h"
#include "error.h"
#include "Exception.h"
#include "fixdat.h"
#include "globals.h"
#include "MemDB.h"
#include "RomDB.h"
#include "sighandle.h"
#include "Stats.h"
#include "superfluous.h"
#include "Tree.h"
#include "util.h"


/* to identify roms directory uniquely */
std::string rom_dir_normalized;

const char help_head[] = PACKAGE " by Dieter Baron and Thomas Klausner";
const char help_footer[] = "Report bugs to " PACKAGE_BUGREPORT ".";
const char version_string[] = PACKAGE " " VERSION "\n"
				"Copyright (C) 1999-2022 Dieter Baron and Thomas Klausner\n" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

std::vector<Commandline::Option> options = {
    Commandline::Option("fix", 'F', "fix ROM set"),
    Commandline::Option("game-list", 'T', "file", "read games to check from file"),
};

std::unordered_set<std::string> used_variables = {
    "complete_games_only",
    "create_fixdat",
    "extra_directories",
    "fixdat_directory",
    "keep_old_duplicate",
    "move_from_extra",
    "old_db",
    "report_correct",
    "report_detailed",
    "report_fixable",
    "report_missing",
    "report_summary",
    "rom_db",
    "rom_directory",
    "roms_zipped",
    "verbose"
};

static bool contains_romdir(const std::string &ame);

int
main(int argc, char **argv) {
    setprogname(argv[0]);

    try {
	int found;
	std::string game_list;
	std::vector<std::string> arguments;
		
	auto commandline = Commandline(options, "[game ...]", help_head, help_footer, version_string);

	Configuration::add_options(commandline, used_variables);

	try {
	    auto args = commandline.parse(argc, argv);

	    configuration.handle_commandline(args);

	    for (const auto &option : args.options) {
		if (option.name == "fix") {
		    configuration.fix_romset = true;
		}
		else if (option.name == "game-list") {
		    game_list = option.argument;
		}
		else {
		    // TODO: report unhandled option
		}
	    }
	    
	    arguments = args.arguments;
	}
	catch (Exception &ex) {
	    commandline.usage(false, stderr);
	    exit(1);
	}

	if (!configuration.fix_romset) {
	    Archive::read_only_mode = true;
	}

	ensure_dir(configuration.rom_directory, false);
	std::error_code ec;
	rom_dir_normalized = std::filesystem::relative(configuration.rom_directory, "/", ec);
	if (ec || rom_dir_normalized.empty()) {
	    /* TODO: treat as warning only? (this exits if any ancestor directory is unreadable) */
	    myerror(ERRSTR, "can't normalize directory '%s'", configuration.rom_directory.c_str());
	    exit(1);
	}

	try {
	    CkmameDB::register_directory(configuration.rom_directory);
	    CkmameDB::register_directory(needed_dir);
	    CkmameDB::register_directory(unknown_dir);
	    for (const auto &name : configuration.extra_directories) {
		if (contains_romdir(name)) {
		    /* TODO: improve error message: also if extra is in ROM directory. */
		    myerror(ERRDEF, "current ROM directory '%s' is in extra directory '%s'", configuration.rom_directory.c_str(), name.c_str());
		    exit(1);
		}
		CkmameDB::register_directory(name);
	    }
	}
	catch (Exception &exception) {
	    exit(1);
	}

	try {
	    db = std::make_unique<RomDB>(configuration.rom_db, DBH_READ);
	}
	catch (std::exception &e) {
	    myerror(0, "can't open database '%s': %s", configuration.rom_db.c_str(), e.what());
	    exit(1);
	}
	try {
	    old_db = std::make_unique<RomDB>(configuration.old_db, DBH_READ);
	}
	catch (std::exception &e) {
	    /* TODO: check for errors other than ENOENT */
	}

	if (configuration.create_fixdat) {
	    DatEntry de;

	    auto d = db->read_dat();

	    if (d.empty()) {
		myerror(ERRDEF, "database error reading /dat");
		exit(1);
	    }

	    auto fixdat_fname = "fixdat_" + d[0].name + " (" + d[0].version + ").dat";
	    if (!configuration.fixdat_directory.empty()) {
		fixdat_fname = configuration.fixdat_directory + "/" + fixdat_fname;
	    }

	    de.name = "Fixdat for " + d[0].name + " (" + d[0].version + ")";
	    de.description = "Fixdat by ckmame";
	    de.version = format_time("%Y-%m-%d %H:%M:%S", time(nullptr));

	    if ((fixdat = OutputContext::create(OutputContext::FORMAT_DATAFILE_XML, fixdat_fname, 0)) == nullptr) {
		exit(1);
	    }

	    fixdat->header(&de);
	}

	if (!configuration.roms_zipped && db->has_disks() == 1) {
	    fprintf(stderr, "%s: unzipped mode is not supported for ROM sets with disks\n", getprogname());
	    exit(1);
	}

	/* build tree of games to check */
	std::vector<std::string> list;
	
	try {
	    list = db->read_list(DBH_KEY_LIST_GAME);
	}
	catch (Exception &e) {
	    myerror(ERRDEF, "list of games not found in database '%s': %s", configuration.rom_db.c_str(), e.what());
	    exit(1);
	}
	std::sort(list.begin(), list.end());

	if (!game_list.empty()) {
	    char b[8192];

	    seterrinfo(game_list);

	    auto f = make_shared_file(game_list, "r");
	    if (!f) {
		myerror(ERRZIPSTR, "cannot open game list");
		exit(1);
	    }

	    while (fgets(b, sizeof(b), f.get())) {
		if (b[strlen(b) - 1] == '\n')
		    b[strlen(b) - 1] = '\0';
		else {
		    myerror(ERRZIP, "overly long line ignored");
		    continue;
		}

		if (std::binary_search(list.begin(), list.end(), b)) {
		    check_tree.add(b);
		}
		else {
		    myerror(ERRDEF, "game '%s' unknown", b);
		}
	    }
	}
	else if (arguments.empty()) {
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
			myerror(ERRDEF, "game '%s' unknown", argument.c_str());
		    }
		}
		else {
		    found = 0;
		    for (const auto & j : list) {
			if (fnmatch(argument.c_str(), j.c_str(), 0) == 0) {
			    check_tree.add(j);
			    found = 1;
			}
		    }
		    if (!found)
			myerror(ERRDEF, "no game matching '%s' found", argument.c_str());
		}
	    }
	}

	MemDB::ensure();
	
	if (!superfluous_delete_list) {
	    superfluous_delete_list = std::make_shared<DeleteList>();
	}
	superfluous_delete_list->add_directory(configuration.rom_directory, true);

	if (configuration.fix_romset) {
	    ensure_extra_maps(DO_MAP | DO_LIST);
	}

    #ifdef SIGINFO
	signal(SIGINFO, sighandle);
    #endif

	check_tree.traverse();
	check_tree.traverse(); /* handle rechecks */

	if (configuration.fix_romset) {
	    if (!needed_delete_list) {
		needed_delete_list = std::make_shared<DeleteList>();
	    }
	    if (needed_delete_list->archives.empty()) {
		needed_delete_list->add_directory(needed_dir, false);
	    }
	    cleanup_list(superfluous_delete_list, CLEANUP_NEEDED | CLEANUP_UNKNOWN);
	    cleanup_list(needed_delete_list, CLEANUP_UNKNOWN, true);
	}

	if (fixdat) {
	    fixdat->close();
	}

	if (configuration.fix_romset && configuration.move_from_extra) {
	    cleanup_list(extra_delete_list, 0);
	}

	if (arguments.empty()) {
	    print_superfluous(superfluous_delete_list);
	}
	
	if (configuration.report_summary) {
	    stats.print(stdout, false);
	}

	CkmameDB::close_all();
	
	if (configuration.fix_romset) {
	    std::error_code ec;
	    std::filesystem::remove(needed_dir, ec);
	}

	db = nullptr;
	old_db = nullptr;

	return 0;
    }
    catch (const std::exception &e) {
	fprintf(stderr, "%s: unexpected error: %s\n", getprogname(), e.what());
	exit(1);
    }
}


static bool
contains_romdir(const std::string &name) {
    std::error_code ec;
    std::string normalized = std::filesystem::relative(name, "/", ec);
    if (ec || normalized.empty()) {
	return false;
    }

    size_t length = std::min(normalized.length(), rom_dir_normalized.length());
    return (normalized.substr(0, length) == rom_dir_normalized.substr(0, length));
}
