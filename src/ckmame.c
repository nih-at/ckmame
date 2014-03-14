/*
  ckmame.c -- main routine for ckmame
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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


#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "compat.h"
#include "dbh.h"
#include "dbh_dir.h"
#include "error.h"
#include "globals.h"
#include "funcs.h"
#include "sighandle.h"
#include "tree.h"
#include "types.h"
#include "util.h"
#include "warn.h"
#include "xmalloc.h"

enum action {
    ACTION_UNSPECIFIED,
    ACTION_CHECK_ROMSET,
    ACTION_SUPERFLUOUS_ONLY,
    ACTION_CLEANUP_EXTRA_ONLY
};

typedef enum action action_t;


char *usage = "Usage: %s [-bcdFfhjKkLlnSsuVvwX] [-D dbfile] [-O dbfile] [-e dir] [-R dir] [-T file] [game...]\n";

char help_head[] = PACKAGE " by Dieter Baron and Thomas Klausner\n\n";

char help[] = "\n"
"  -b, --nobroken          don't report unfixable errors\n"
"      --cleanup-extra     clean up extra dirs (delete superfluous files)\n"
"  -c, --correct           report correct sets\n"
"  -D, --db dbfile         use mame-db dbfile\n"
"  -d, --nonogooddumps     don't report roms with no good dumps\n"
"  -e, --search dir        search for missing files in directory dir\n"
"  -F, --fix               fix rom sets\n"
"      --fixdat datfile    write fixdat to `datfile'\n"
"  -f, --nofixable         don't report fixable errors\n"
"  -h, --help              display this help message\n"
"  -I, --ignore-unknown    do not touch unknown files when fixing\n"
"  -i, --integrity         check integrity of rom files and disk images\n"
"      --keep-found        keep files copied from search directory (default)\n"
"  -j, --delete-found      delete files copied from search directories\n"
"      --keep-duplicate    keep files present in old rom db\n"
"      --delete-duplicate  delete files present in old rom db (default)\n"
"  -K, --move-unknown      move unknown files when fixing (default)\n"
"  -k, --delete-unknown    delete unknown files when fixing\n"
"  -L, --move-long         move long files when fixing (default)\n"
"  -l, --delete-long       delete long files when fixing\n"
"  -n, --dryrun            don't actually fix, only report what would be done\n"
"  -O, --old-db dbfile     use mame-db dbfile for old roms\n"
"  -R, --rom-dir dir       look for roms in rom-dir (default: 'roms')\n"
"  -S, --samples           check samples instead of roms\n"
"      --superfluous       only check for superfluous files in rom sets\n"
"  -s, --nosuperfluous     don't report superfluous files in rom sets\n"
"      --torrentzip        TorrentZip ROM archives\n"
"  -T, --games-from file   read games to check from file\n"
"  -u, --roms-unzipped     ROMs are files on disk, not contained in zip archives\n"
"  -V, --version           display version number\n"
"  -v, --verbose           print fixes made\n"
"  -w, --nowarnings        print only unfixable errors\n"
"  -X, --ignore-extra      ignore extra files in rom/samples dirs\n"
"\nReport bugs to " PACKAGE_BUGREPORT ".\n";

char version_string[] = PACKAGE " " VERSION "\n"
"Copyright (C) 2014 Dieter Baron and Thomas Klausner\n"
PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

#define OPTIONS "bcD:de:FfhijKkLlnO:R:SsT:uVvwX"

enum {
    OPT_CLEANUP_EXTRA = 256,
    OPT_DELETE_DUPLICATE,
    OPT_FIXDAT,
    OPT_IGNORE_UNKNOWN,
    OPT_KEEP_DUPLICATE,
    OPT_KEEP_FOUND,
    OPT_SUPERFLUOUS,
    OPT_TORRENTZIP
};

struct option options[] = {
    { "help",              0, 0, 'h' },
    { "version",           0, 0, 'V' },

    { "cleanup-extra",     0, 0, OPT_CLEANUP_EXTRA },
    { "correct",           0, 0, 'c' }, /* +CORRECT */
    { "db",                1, 0, 'D' },
    { "delete-duplicate",  0, 0, OPT_DELETE_DUPLICATE }, /*+DELETE_DUPLICATE */
    { "delete-found",      0, 0, 'j' },
    { "delete-long",       0, 0, 'l' },
    { "delete-unknown",    0, 0, 'k' },
    { "dryrun",            0, 0, 'n' },
    { "fix",               0, 0, 'F' },
    { "fixdat",            1, 0, OPT_FIXDAT },
    { "games-from",        1, 0, 'T' },
    { "ignore-extra",      0, 0, 'X' },
    { "ignore-unknown",    0, 0, OPT_IGNORE_UNKNOWN },
    { "integrity",         0, 0, 'i' },
    { "keep-duplicate",    0, 0, OPT_KEEP_DUPLICATE }, /* -DELETE_DUPLICATE */
    { "keep-found",        0, 0, OPT_KEEP_FOUND },
    { "move-long",         0, 0, 'L' },
    { "move-unknown",      0, 0, 'K' },
    { "nobroken",          0, 0, 'b' }, /* -BROKEN */
    { "nofixable",         0, 0, 'f' }, /* -FIX */
    { "nonogooddumps",     0, 0, 'd' }, /* -NO_GOOD_DUMPS */
    { "nosuperfluous",     0, 0, 's' }, /* -SUP */
    { "nowarnings",        0, 0, 'w' }, /* -SUP, -FIX */
    { "old-db",            1, 0, 'O' },
    { "rom-dir",           1, 0, 'R' },
    { "roms-unzipped",     0, 0, 'u' },
    { "samples",           0, 0, 'S' },
    { "search",            1, 0, 'e' },
    { "superfluous",       0, 0, OPT_SUPERFLUOUS },
    { "torrentzip",        0, 0, OPT_TORRENTZIP },
    { "verbose",           0, 0, 'v' },

    { NULL,                0, 0, 0 },
};

static int ignore_extra;


static void error_multiple_actions(void);


int
main(int argc, char **argv)
{
    action_t action;
    int i, j;
    char *dbname, *olddbname;
    int c, found;
    parray_t *list;
    char *game_list;
    
    setprogname(argv[0]);
    output_options = WARN_ALL;
    file_type = TYPE_ROM;
    action = ACTION_UNSPECIFIED;
    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = DBH_DEFAULT_DB_NAME;
    olddbname = getenv("MAMEDB_OLD");
    if (olddbname == NULL)
	olddbname = DBH_DEFAULT_OLD_DB_NAME;
    fix_options = FIX_MOVE_LONG | FIX_MOVE_UNKNOWN | FIX_DELETE_DUPLICATE;
    ignore_extra = 0;
    check_integrity = 0;
    roms_unzipped = 0;
    search_dirs = parray_new();
    game_list = NULL;
    rom_dir = NULL;
    fixdat = NULL;
    
    opterr = 0;
    while ((c=getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
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
	    output_options &= ~WARN_BROKEN;
	    break;
	case 'c':
	    output_options |= WARN_CORRECT;
	    break;
	case 'D':
	    dbname = optarg;
	    break;
	case 'd':
	    output_options &= ~WARN_NO_GOOD_DUMP;
	    break;
	case 'e':
	    parray_push(search_dirs, xstrdup(optarg));
	    break;
	case 'F':
	    fix_options |= FIX_DO;
	    break;
	case 'f':
	    output_options &= ~WARN_FIXABLE;
	    break;
	case 'i':
	    check_integrity = 1;
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
	case 'n':
	    fix_options &= ~FIX_DO;
	    fix_options |= FIX_PRINT;
	    break;
	case 'O':
	    olddbname = optarg;
	    break;
	case 'R':
	    rom_dir = optarg;
	    break;
	case 'S':
	    file_type = TYPE_SAMPLE;
	    break;
	case 's':
	    output_options &= ~WARN_SUPERFLUOUS;
	    break;
	case 'T':
	    game_list = optarg;
            break;
        case 'u':
            roms_unzipped = 1;
	    break;
	case 'v':
	    fix_options |= FIX_PRINT;
	    break;
	case 'w':
	    output_options &= WARN_BROKEN;
	    break;
	case 'X':
	    ignore_extra = 1;
	    break;
	case OPT_CLEANUP_EXTRA:
	    if (action != ACTION_UNSPECIFIED)
		error_multiple_actions();
	    action = ACTION_CLEANUP_EXTRA_ONLY;
	    fix_options |= FIX_DO|FIX_CLEANUP_EXTRA;
	    break;
	case OPT_DELETE_DUPLICATE:
	    fix_options |= FIX_DELETE_DUPLICATE;
	    break;
	case OPT_FIXDAT:
	    {
		dat_entry_t de;

		de.name = "Fixdat";
		de.description = "Fixdat by ckmame";
		de.version = "1";
		    
		if ((fixdat=output_new(OUTPUT_FMT_DATAFILE_XML, optarg)) == NULL)
		    exit(1);

		output_header(fixdat, &de);
	    }
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
	case OPT_SUPERFLUOUS:
	    if (action != ACTION_UNSPECIFIED)
		error_multiple_actions();
	    action = ACTION_SUPERFLUOUS_ONLY;
	    break;
	case OPT_TORRENTZIP:
	    fix_options |= FIX_TORRENTZIP;
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

    if (file_type == TYPE_ROM) {
	archive_register_cache_directory(get_directory(file_type));
	archive_register_cache_directory(needed_dir);
	/* archive_register_cache_directory(unknown_dir); */
	int i;
	for (i=0; i<parray_length(search_dirs); i++) {
	    if (archive_register_cache_directory(parray_get(search_dirs, i)) < 0)
		exit(1);
	}
    }

    if ((db=romdb_open(dbname, DBH_READ)) == NULL) {
	myerror(ERRSTR, "can't open database '%s'", dbname);
	exit(1);
    }
    /* TODO: check for errors other than ENOENT */
    old_db = romdb_open(olddbname, DBH_READ);

    if (action == ACTION_CHECK_ROMSET) {
	/* build tree of games to check */

	if ((list=romdb_read_list(db, DBH_KEY_LIST_GAME)) == NULL) {
	    myerror(ERRDEF,
		    "list of games not found in database '%s'", dbname);
	    exit(1);
	}

	check_tree = tree_new();

	if (game_list) {
	    FILE *f;
	    char b[8192];

	    seterrinfo(game_list, NULL);
	    
	    if ((f=fopen(game_list, "r")) == NULL) {
		myerror(ERRZIPSTR, "cannot open game list");
		exit(1);
	    }

	    while (fgets(b, sizeof(b), f)) {
		if (b[strlen(b)-1] == '\n')
		    b[strlen(b)-1] = '\0';
		else {
		    myerror(ERRZIP, "overly long line ignored");
		    continue;
		}

		if (parray_index_sorted(list, b, strcmp) >= 0)
		    tree_add(check_tree, b);
		else
		    myerror(ERRDEF, "game '%s' unknown", b);
	    }

	    fclose(f);
	}
	else if (optind == argc) {
	    for (i=0; i<parray_length(list); i++)
		tree_add(check_tree, parray_get(list, i));
	}
	else {
	    for (i=optind; i<argc; i++) {
		if (strcspn(argv[i], "*?[]{}") == strlen(argv[i])) {
		    if (parray_index_sorted(list, argv[i], strcmp) >= 0)
			tree_add(check_tree, argv[i]);
		    else
			myerror(ERRDEF, "game '%s' unknown", argv[i]);
		}
		else {
		    found = 0;
		    for (j=0; j<parray_length(list); j++) {
			if (fnmatch(argv[i], parray_get(list, j), 0) == 0) {
			    tree_add(check_tree, parray_get(list, j));
			    found = 1;
			}
		    }
		    if (!found)
			myerror(ERRDEF,
				"no game matching '%s' found", argv[i]);
		}
	    }
	}

	parray_free(list, free);
    }

    if (action != ACTION_SUPERFLUOUS_ONLY) {
	/* TODO: merge in olddb */
	detector = romdb_read_detector(db);
    }
    
    if (action != ACTION_CLEANUP_EXTRA_ONLY)
	superfluous = list_directory(get_directory(file_type), dbname);

    if ((fix_options & (FIX_DO|FIX_PRINT)) && (fix_options & FIX_CLEANUP_EXTRA))
	ensure_extra_maps((action==ACTION_CHECK_ROMSET ? DO_MAP : 0) | DO_LIST);

#ifdef SIGINFO
    signal(SIGINFO, sighandle);
#endif

    if (action == ACTION_CHECK_ROMSET) {
	tree_traverse(check_tree, NULL, NULL);
	tree_traverse(check_tree, NULL, NULL);

	if (fix_options & (FIX_DO|FIX_PRINT)) {
	    if (fix_options & FIX_SUPERFLUOUS) {
		parray_t *needed_list;

		cleanup_list(superfluous, superfluous_delete_list,
			     CLEANUP_NEEDED|CLEANUP_UNKNOWN);
		needed_list = list_directory(needed_dir, NULL);
		cleanup_list(needed_list, needed_delete_list,
			     CLEANUP_UNKNOWN);
            }
	    else {
	        if (needed_delete_list)
		    delete_list_execute(needed_delete_list);
		if (superfluous_delete_list)
		    delete_list_execute(superfluous_delete_list);
	    }
	}
    }

    if (fixdat)
	output_close(fixdat);

    if ((fix_options & (FIX_DO|FIX_PRINT))
	&& (fix_options & FIX_CLEANUP_EXTRA))
	cleanup_list(extra_list, extra_delete_list, 0);
    else if (extra_delete_list)
	delete_list_execute(extra_delete_list);

    if ((action == ACTION_CHECK_ROMSET &&
	 (optind == argc && (output_options & WARN_SUPERFLUOUS)))
	|| action == ACTION_SUPERFLUOUS_ONLY)
	print_superfluous(superfluous);

    if (roms_unzipped)
	dbh_dir_close_all();

    return 0;
}


static void
error_multiple_actions(void)
{
    fprintf(stderr, "%s: only one of --cleanup-extra, --superfluous, "
	    "game can be used\n", getprogname());
    exit(1);
}
