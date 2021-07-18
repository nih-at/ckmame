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

#include "compat.h"
#include "config.h"

#include "check_util.h"
#include "cleanup.h"
#include "CkmameDB.h"
#include "diagnostics.h"
#include "error.h"
#include "Exception.h"
#include "fix.h"
#include "fixdat.h"
#include "globals.h"
#include "RomDB.h"
#include "sighandle.h"
#include "Stats.h"
#include "superfluous.h"
#include "Tree.h"
#include "util.h"
#include "warn.h"


enum action { ACTION_UNSPECIFIED, ACTION_CHECK_ROMSET, ACTION_SUPERFLUOUS_ONLY, ACTION_CLEANUP_EXTRA_ONLY };

typedef enum action action_t;

/* to identify roms directory uniquely */
std::string rom_dir_normalized;

const char *usage = "Usage: %s [-bCcdFfhjKkLlSsuVvwX] [-D dbfile] [-O dbfile] [-e dir] [-R dir] [-T file] [game...]\n";

const char help_head[] = PACKAGE " by Dieter Baron and Thomas Klausner\n\n";

const char help[] = "\n"
	      "      --autofixdat        write fixdat to `fix_$NAME_OF_SET.dat'\n"
	      "  -b, --nobroken          don't report unfixable errors\n"
	      "      --cleanup-extra     clean up extra dirs (delete superfluous files)\n"
	      "  -C, --complete-only     only keep complete sets in rom-dir\n"
	      "  -c, --correct           report correct sets\n"
	      "  -D, --db dbfile         use mame-db dbfile\n"
	      "  -d, --nonogooddumps     don't report roms with no good dumps\n"
	      "  -e, --search dir        search for missing files in directory dir\n"
	      "  -F, --fix               fix rom sets\n"
	      "      --fixdat datfile    write fixdat to `datfile'\n"
	      "  -f, --nofixable         don't report fixable errors\n"
	      "  -h, --help              display this help message\n"
	      "  -I, --ignore-unknown    do not touch unknown files when fixing\n"
	      "      --keep-found        keep files copied from search directory (default)\n"
	      "  -j, --delete-found      delete files copied from search directories\n"
	      "      --keep-duplicate    keep files present in old rom db\n"
	      "      --delete-duplicate  delete files present in old rom db (default)\n"
	      "  -K, --move-unknown      move unknown files when fixing (default)\n"
	      "  -k, --delete-unknown    delete unknown files when fixing\n"
	      "  -L, --move-long         move long files when fixing (default)\n"
	      "  -l, --delete-long       delete long files when fixing\n"
	      "  -O, --old-db dbfile     use mame-db dbfile for old roms\n"
	      "  -R, --rom-dir dir       look for roms in rom-dir (default: 'roms')\n"
	      "      --stats             print stats of checked ROMs\n"
	      "      --superfluous       only check for superfluous files in rom sets\n"
	      "  -s, --nosuperfluous     don't report superfluous files in rom sets\n"
	      "  -T, --games-from file   read games to check from file\n"
	      "  -u, --roms-unzipped     ROMs are files on disk, not contained in zip archives\n"
	      "  -V, --version           display version number\n"
	      "  -v, --verbose           print fixes made\n"
	      "  -w, --nowarnings        print only unfixable errors\n"
	      "  -X, --ignore-extra      ignore extra files in rom dirs\n"
	      "\nReport bugs to " PACKAGE_BUGREPORT ".\n";

const char version_string[] = PACKAGE " " VERSION "\n"
				"Copyright (C) 1999-2018 Dieter Baron and Thomas Klausner\n" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

#define OPTIONS "bCcD:de:FfhjKkLlO:R:SsT:uVvwX"

enum { OPT_CLEANUP_EXTRA = 256, OPT_DELETE_DUPLICATE, OPT_AUTOFIXDAT, OPT_FIXDAT, OPT_IGNORE_UNKNOWN, OPT_KEEP_DUPLICATE, OPT_KEEP_FOUND, OPT_SUPERFLUOUS, OPT_STATS };

struct option options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'V'},

    {"autofixdat", 0, 0, OPT_AUTOFIXDAT},
    {"cleanup-extra", 0, 0, OPT_CLEANUP_EXTRA},
    {"complete-only", 0, 0, 'C'},
    {"correct", 0, 0, 'c'}, /* +CORRECT */
    {"db", 1, 0, 'D'},
    {"delete-duplicate", 0, 0, OPT_DELETE_DUPLICATE}, /*+DELETE_DUPLICATE */
    {"delete-found", 0, 0, 'j'},
    {"delete-long", 0, 0, 'l'},
    {"delete-unknown", 0, 0, 'k'},
    {"fix", 0, 0, 'F'},
    {"fixdat", 1, 0, OPT_FIXDAT},
    {"games-from", 1, 0, 'T'},
    {"ignore-extra", 0, 0, 'X'},
    {"ignore-unknown", 0, 0, OPT_IGNORE_UNKNOWN},
    {"keep-duplicate", 0, 0, OPT_KEEP_DUPLICATE}, /* -DELETE_DUPLICATE */
    {"keep-found", 0, 0, OPT_KEEP_FOUND},
    {"move-long", 0, 0, 'L'},
    {"move-unknown", 0, 0, 'K'},
    {"nobroken", 0, 0, 'b'},      /* -BROKEN */
    {"nofixable", 0, 0, 'f'},     /* -FIX */
    {"nonogooddumps", 0, 0, 'd'}, /* -NO_GOOD_DUMPS */
    {"nosuperfluous", 0, 0, 's'}, /* -SUP */
    {"nowarnings", 0, 0, 'w'},    /* -SUP, -FIX */
    {"old-db", 1, 0, 'O'},
    {"rom-dir", 1, 0, 'R'},
    {"roms-unzipped", 0, 0, 'u'},
    {"search", 1, 0, 'e'},
    {"stats", 0, 0, OPT_STATS},
    {"superfluous", 0, 0, OPT_SUPERFLUOUS},
    {"verbose", 0, 0, 'v'},

    {NULL, 0, 0, 0},
};

static int ignore_extra;


static bool contains_romdir(std::string &ame);
static void error_multiple_actions(void);


int
main(int argc, char **argv) {
    setprogname(argv[0]);

    try {
	action_t action;
	const char *dbname, *olddbname;
	int c, found;
	std::string fixdat_name;
	char *game_list;
	bool auto_fixdat;
	bool print_stats = false;

	diagnostics_options = WARN_ALL;
	action = ACTION_UNSPECIFIED;
	dbname = getenv("MAMEDB");
	if (dbname == NULL)
	    dbname = DBH_DEFAULT_DB_NAME;
	olddbname = getenv("MAMEDB_OLD");
	if (olddbname == NULL)
	    olddbname = DBH_DEFAULT_OLD_DB_NAME;
	fix_options = FIX_MOVE_LONG | FIX_MOVE_UNKNOWN | FIX_DELETE_DUPLICATE;
	ignore_extra = 0;
	roms_unzipped = false;
	game_list = NULL;
	fixdat = NULL;
	auto_fixdat = false;

	opterr = 0;
	while ((c = getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
	    switch (c) {
	    case 'h':
		fputs(help_head, stdout);
		printf(usage, getprogname());
		fputs(help, stdout);
		exit(0);
	    case 'V':
		fputs(version_string, stdout);
		exit(0);

	    case 'b':
		diagnostics_options &= ~WARN_BROKEN;
		break;
	    case 'C':
		fix_options |= FIX_COMPLETE_ONLY;
		break;
	    case 'c':
		diagnostics_options |= WARN_CORRECT;
		break;
	    case 'D':
		dbname = optarg;
		break;
	    case 'd':
		diagnostics_options &= ~WARN_NO_GOOD_DUMP;
		break;
	    case 'e': {
		std::string name = optarg;
		auto last = name.find_last_not_of("/");
		if (last == std::string::npos) {
		    name = "/";
		}
		else {
		    name.resize(last + 1);
		}
		search_dirs.push_back(name);
		break;
	    }
	    case 'F':
		fix_options |= FIX_DO;
		break;
	    case 'f':
		diagnostics_options &= ~WARN_FIXABLE;
		break;
	    case 'j':
		fix_options |= FIX_DELETE_EXTRA;
		break;
	    case 'K':
		fix_options |= FIX_MOVE_UNKNOWN;
		break;
	    case 'k':
		fix_options &= ~FIX_MOVE_UNKNOWN;
		break;
	    case 'L':
		fix_options |= FIX_MOVE_LONG;
		break;
	    case 'l':
		fix_options &= ~FIX_MOVE_LONG;
		break;
	    case 'O':
		olddbname = optarg;
		break;
	    case 'R':
		rom_dir = optarg;
		break;
	    case 's':
		diagnostics_options &= ~WARN_SUPERFLUOUS;
		break;
	    case 'T':
		game_list = optarg;
		break;
	    case 'u':
		roms_unzipped = true;
		break;
	    case 'v':
		fix_options |= FIX_PRINT;
		break;
	    case 'w':
		diagnostics_options &= WARN_BROKEN;
		break;
	    case 'X':
		ignore_extra = 1;
		break;
	    case OPT_CLEANUP_EXTRA:
		if (action != ACTION_UNSPECIFIED)
		    error_multiple_actions();
		action = ACTION_CLEANUP_EXTRA_ONLY;
		fix_options |= FIX_DO | FIX_CLEANUP_EXTRA;
		break;
	    case OPT_DELETE_DUPLICATE:
		fix_options |= FIX_DELETE_DUPLICATE;
		break;
	    case OPT_AUTOFIXDAT:
		auto_fixdat = true;
		break;
	    case OPT_FIXDAT:
		fixdat_name = optarg;
		break;
	    case OPT_IGNORE_UNKNOWN:
		fix_options |= FIX_IGNORE_UNKNOWN;
		break;
	    case OPT_KEEP_DUPLICATE:
		fix_options &= ~FIX_DELETE_DUPLICATE;
		break;
	    case OPT_KEEP_FOUND:
		fix_options &= ~FIX_DELETE_EXTRA;
		break;
	    case OPT_STATS:
		print_stats = true;
		break;
	    case OPT_SUPERFLUOUS:
		if (action != ACTION_UNSPECIFIED)
		    error_multiple_actions();
		action = ACTION_SUPERFLUOUS_ONLY;
		break;
	    default:
		fprintf(stderr, usage, getprogname());
		exit(1);
	    }
	}

	if ((fix_options & FIX_DO) == 0)
	    archive_global_flags(ARCHIVE_FL_RDONLY, true);

	if (optind != argc) {
	    if (action != ACTION_UNSPECIFIED)
		error_multiple_actions();
	    action = ACTION_CHECK_ROMSET;
	}
	else if (game_list) {
	    if (action != ACTION_UNSPECIFIED)
		error_multiple_actions();
	    action = ACTION_CHECK_ROMSET;
	}
	else if (action == ACTION_UNSPECIFIED) {
	    action = ACTION_CHECK_ROMSET;
	    fix_options |= FIX_SUPERFLUOUS;
	    if (fix_options & FIX_DELETE_EXTRA)
		fix_options |= FIX_CLEANUP_EXTRA;
	}

	ensure_dir(get_directory(), false);
	std::error_code ec;
	rom_dir_normalized = std::filesystem::relative(get_directory(), "/", ec);
	if (ec || rom_dir_normalized.empty()) {
	    /* TODO: treat as warning only? (this exits if any ancestor directory is unreadable) */
	    myerror(ERRSTR, "can't normalize directory '%s'", get_directory().c_str());
	    exit(1);
	}

	try {
	    CkmameDB::register_directory(get_directory());
	    CkmameDB::register_directory(needed_dir);
	    CkmameDB::register_directory(unknown_dir);
	    for (size_t m = 0; m < search_dirs.size(); m++) {
		auto name = search_dirs[m];
		if (contains_romdir(name)) {
		    /* TODO: improve error message: also if extra is in ROM directory. */
		    myerror(ERRDEF, "current ROM directory '%s' is in extra directory '%s'", get_directory().c_str(), name.c_str());
		    exit(1);
		}
		CkmameDB::register_directory(name);
	    }
	}
	catch (Exception &exception) {
	    exit(1);
	}

	try {
	    db = std::make_unique<RomDB>(dbname, DBH_READ);
	}
	catch (std::exception &e) {
	    myerror(0, "can't open database '%s': %s", dbname, e.what());
	    exit(1);
	}
	try {
	    old_db = std::make_unique<RomDB>(olddbname, DBH_READ);
	}
	catch (std::exception &e) {
	    /* TODO: check for errors other than ENOENT */
	}

	if (auto_fixdat || !fixdat_name.empty()) {
	    DatEntry de;

	    if (auto_fixdat) {
		if (!fixdat_name.empty()) {
		    myerror(ERRDEF, "do not use --autofixdat and --fixdat together");
		    exit(1);
		}

		auto d = db->read_dat();;

		if (d.empty()) {
		    myerror(ERRDEF, "database error reading /dat");
		    exit(1);
		}

		fixdat_name = "fix_" + d[0].name + " (" + d[0].version + ").dat";
	    }

	    de.name = "Fixdat";
	    de.description = "Fixdat by ckmame";
	    de.version = "1";

	    if ((fixdat = OutputContext::create(OutputContext::FORMAT_DATAFILE_XML, fixdat_name, 0)) == NULL) {
		exit(1);
	    }

	    fixdat->header(&de);
	}

	if (roms_unzipped && db->has_disks() == 1) {
	    fprintf(stderr, "%s: unzipped mode is not supported for ROM sets with disks\n", getprogname());
	    exit(1);
	}

	if (action == ACTION_CHECK_ROMSET) {
	    /* build tree of games to check */
	    auto list = db->read_list(DBH_KEY_LIST_GAME);
	    if (list.empty()) {
		myerror(ERRDEF, "list of games not found in database '%s'", dbname);
		exit(1);
	    }
	    std::sort(list.begin(), list.end());

	    if (game_list) {
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
	    else if (optind == argc) {
		for (size_t i = 0; i < list.size(); i++) {
		    check_tree.add(list[i]);
		}
	    }
	    else {
		for (auto i = optind; i < argc; i++) {
		    if (strcspn(argv[i], "*?[]{}") == strlen(argv[i])) {
			if (std::binary_search(list.begin(), list.end(), argv[i])) {
			    check_tree.add(argv[i]);
			}
			else {
			    myerror(ERRDEF, "game '%s' unknown", argv[i]);
			}
		    }
		    else {
			found = 0;
			for (size_t j = 0; j < list.size(); j++) {
			    if (fnmatch(argv[i], list[j].c_str(), 0) == 0) {
				check_tree.add(list[j]);
				found = 1;
			    }
			}
			if (!found)
			    myerror(ERRDEF, "no game matching '%s' found", argv[i]);
		    }
		}
	    }
	}

	if (action != ACTION_CLEANUP_EXTRA_ONLY) {
	    if (!superfluous_delete_list) {
		superfluous_delete_list = std::make_shared<DeleteList>();
	    }
	    superfluous_delete_list->add_directory(get_directory(), true);
	}

	if ((fix_options & FIX_DO) && (fix_options & FIX_CLEANUP_EXTRA)) {
	    ensure_extra_maps((action == ACTION_CHECK_ROMSET ? DO_MAP : 0) | DO_LIST);
	}

    #ifdef SIGINFO
	signal(SIGINFO, sighandle);
    #endif

	if (action == ACTION_CHECK_ROMSET) {
	    check_tree.traverse();
	    check_tree.traverse(); /* handle rechecks */

	    if (fix_options & FIX_DO) {
		if (fix_options & FIX_SUPERFLUOUS) {
		    if (!needed_delete_list) {
			needed_delete_list = std::make_shared<DeleteList>();
		    }
		    if (needed_delete_list->archives.empty()) {
			needed_delete_list->add_directory(needed_dir, false);
		    }
		    cleanup_list(superfluous_delete_list, CLEANUP_NEEDED | CLEANUP_UNKNOWN);
		    cleanup_list(needed_delete_list, CLEANUP_UNKNOWN);
		}
		else {
		    if (needed_delete_list) {
			needed_delete_list->execute();
		    }
		    if (superfluous_delete_list) {
			superfluous_delete_list->execute();
		    }
		}
	    }
	}

	if (fixdat) {
	    fixdat->close();
	}

	if ((fix_options & FIX_DO) && (fix_options & FIX_CLEANUP_EXTRA)) {
	    cleanup_list(extra_delete_list, 0);
	}
	else if (extra_delete_list) {
	    extra_delete_list->execute();
	}

	if ((action == ACTION_CHECK_ROMSET && (optind == argc && (diagnostics_options & WARN_SUPERFLUOUS))) || action == ACTION_SUPERFLUOUS_ONLY) {
	    print_superfluous(superfluous_delete_list);
	}
	
	if (print_stats) {
	    stats.print(stdout, false);
	}

	CkmameDB::close_all();
	
	if ((fix_options & FIX_DO) != 0) {
	    std::error_code ec;
	    std::filesystem::remove(needed_dir, ec);
	}

	db = NULL;
	old_db = NULL;

	return 0;
    }
    catch (const std::exception &e) {
	fprintf(stderr, "%s: unexpected error: %s\n", getprogname(), e.what());
    }
}


static void
error_multiple_actions(void) {
    fprintf(stderr,
	    "%s: only one of --cleanup-extra, --superfluous, "
	    "game can be used\n",
	    getprogname());
    exit(1);
}


static bool
contains_romdir(std::string &name) {
    std::error_code ec;
    std::string normalized = std::filesystem::relative(name, "/", ec);
    if (ec || normalized.empty()) {
	return false;
    }

    size_t length = std::min(normalized.length(), rom_dir_normalized.length());
    return (normalized.substr(0, length) == rom_dir_normalized.substr(0, length));
}
