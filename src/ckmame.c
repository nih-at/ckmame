/*
  $NiH: ckmame.c,v 1.15 2006/05/04 07:52:45 dillo Exp $

  ckmame.c -- main routine for ckmame
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fnmatch.h>

#include "config.h"

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "getopt.h"
#endif

#include "dbh.h"
#include "error.h"
#include "globals.h"
#include "funcs.h"
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



char *prg;

char *usage = "Usage: %s [-bcdFfhjKkLlnSsVvw] [-D dbfile] [-O dbfile] [-e dir] [-T file] [game...]\n";

char help_head[] = PACKAGE " by Dieter Baron and Thomas Klausner\n\n";

char help[] = "\n"
"  -b, --nobroken        don't report unfixable errors\n"
"      --cleanup-extra   clean up extra dirs (delete superfluous files)\n"
"  -c, --correct         report correct sets\n"
"  -D, --db dbfile       use mame-db dbfile\n"
"  -d, --nonogooddumps   don't report roms with no good dumps\n"
"  -e, --search dir      search for missing files in directory dir\n"
"  -F, --fix             fix rom sets\n"
"  -f, --nofixable       don't report fixable errors\n"
"  -h, --help            display this help message\n"
"  -i, --integrity       check integrity of rom files and disk images\n"
"      --keep-found      keep files copied from search directories (default)\n"
"  -j, --delete-found    delete files copied from search directories\n"
"  -K, --move-unknown    move unknown files when fixing (default)\n"
"  -k, --delete-unknown  delete unknown files when fixing\n"
"  -L, --move-long       move long files when fixing (default)\n"
"  -l, --delete-long     delete long files when fixing\n"
"  -n, --dryrun          don't actually fix, only report what would be done\n"
"  -O, --old-db dbfile   use mame-db dbfile for old roms\n"
"  -S, --samples         check samples instead of roms\n"
"      --superfuous      only check for superfluous files in rom sets\n"
"  -s, --nosuperfluous   don't report superfluous files in rom sets\n"
"  -T, --games-from file read games to check from file\n"
"  -V, --version         display version number\n"
"  -v, --verbose         print fixes made\n"
"  -w, --nowarnings      print only unfixable errors\n"
"  -X, --ignoreextra     ignore extra files in rom/samples dirs\n"
"\nReport bugs to <nih@giga.or.at>.\n";

char version_string[] = PACKAGE " " VERSION "\n"
"Copyright (C) 2006 Dieter Baron and Thomas Klausner\n"
PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n"
"You may redistribute copies of\n"
PACKAGE " under the terms of the GNU General Public License.\n"
"For more information about these matters, see the files named COPYING.\n";

#define OPTIONS "bcD:de:FfhijKkLlnO:SsT:xVvwX"

enum {
    OPT_CLEANUP_EXTRA = 256,
    OPT_KEEP_FOUND,
    OPT_SUPERFLUOUS
};

struct option options[] = {
    { "help",          0, 0, 'h' },
    { "version",       0, 0, 'V' },

    { "cleanup-extra", 0, 0, OPT_CLEANUP_EXTRA },
    { "correct",       0, 0, 'c' }, /* +CORRECT */
    { "db",            1, 0, 'D' },
    { "delete-found",  0, 0, 'j' },
    { "delete-long",   0, 0, 'l' },
    { "delete-unknown",0, 0, 'k' },
    { "dryrun",        0, 0, 'n' },
    { "fix",           0, 0, 'F' },
    { "games-from",    1, 0, 'T' },
    { "ignoreextra",   0, 0, 'X' },
    { "integrity",     0, 0, 'i' },
    { "keep-found",    0, 0, OPT_KEEP_FOUND },
    { "move-long",     0, 0, 'L' },
    { "move-unknown",  0, 0, 'K' },
    { "nobroken",      0, 0, 'b' }, /* -BROKEN */
    { "nofixable",     0, 0, 'f' }, /* -FIX */
    { "nonogooddumps", 0, 0, 'd' }, /* -NO_GOOD_DUMPS */
    { "nosuperfluous", 0, 0, 's' }, /* -SUP */
    { "nowarnings",    0, 0, 'w' }, /* -SUP, -FIX */
    { "old-db",        1, 0, 'O' },
    { "samples",       0, 0, 'S' },
    { "search",        1, 0, 'e' },
    { "superfluous",   0, 0, OPT_SUPERFLUOUS },
    { "verbose",       0, 0, 'v' },

    { NULL,            0, 0, 0 },
};

int output_options;
int fix_options;
int ignore_extra;
int romhashtypes, diskhashtypes;
int check_integrity;
parray_t *superfluous;
parray_t *superfluous;
parray_t *search_dirs;
filetype_t file_type;
DB *db;
DB *old_db;



static void error_multiple_actions(void);



int
main(int argc, char **argv)
{
    action_t action;
    int i, j;
    char *dbname, *olddbname;
    int c, found;
    parray_t *list;
    tree_t *tree;
    char *game_list;
    
    prg = argv[0];
    output_options = WARN_ALL;
    file_type = TYPE_ROM;
    action = ACTION_UNSPECIFIED;
    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = DBH_DEFAULT_DB_NAME;
    olddbname = getenv("MAMEDB_OLD");
    if (olddbname == NULL)
	olddbname = DBH_DEFAULT_OLD_DB_NAME;
    fix_options = FIX_MOVE_LONG | FIX_MOVE_UNKNOWN;
    ignore_extra = 0;
    check_integrity = 0;
    search_dirs = parray_new();
    game_list = NULL;
    
    opterr = 0;
    while ((c=getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
	switch (c) {
	case 'h':
	    fputs(help_head, stdout);
	    printf(usage, prg);
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
	case 'S':
	    file_type = TYPE_SAMPLE;
	    break;
	case 's':
	    output_options &= ~WARN_SUPERFLUOUS;
	    break;
	case 'T':
	    game_list = optarg;
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
	case OPT_KEEP_FOUND:
	    fix_options &= ~FIX_DELETE_EXTRA;
	    break;
	case OPT_SUPERFLUOUS:
	    if (action != ACTION_UNSPECIFIED)
		error_multiple_actions();
	    action = ACTION_SUPERFLUOUS_ONLY;
	    break;

	default:
	    fprintf(stderr, usage, prg);
	    exit(1);
	}
    }
    
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

    if ((db=dbh_open(dbname, DBL_READ)) == NULL) {
	myerror(ERRDB, "can't open database `%s'", dbname);
	exit(1);
    }
    /* XXX: check for errors other than ENOENT */
    old_db = dbh_open(olddbname, DBL_READ);

    if (action == ACTION_CHECK_ROMSET) {
	/* build tree of games to check */

	if ((list=r_list(db, DBH_KEY_LIST_GAME)) == NULL) {
	    myerror(ERRDEF,
		    "list of games not found in database `%s'", dbname);
	    exit(1);
	}

	tree = tree_new();

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
		    
		tree_add(tree, b);
	    }

	    fclose(f);
	}
	else if (optind == argc) {
	    for (i=0; i<parray_length(list); i++)
		tree_add(tree, parray_get(list, i));
	}
	else {
	    for (i=optind; i<argc; i++) {
		if (strcspn(argv[i], "*?[]{}") == strlen(argv[i])) {
		    if (parray_index_sorted(list, argv[i], strcmp) >= 0)
			tree_add(tree, argv[i]);
		    else
			myerror(ERRDEF, "game `%s' unknown", argv[i]);
		}
		else {
		    found = 0;
		    for (j=0; j<parray_length(list); j++) {
			if (fnmatch(argv[i], parray_get(list, j), 0) == 0) {
			    tree_add(tree, parray_get(list, j));
			    found = 1;
			}
		    }
		    if (!found)
			myerror(ERRDEF,
				"no game matching `%s' found", argv[i]);
		}
	    }
	}

	parray_free(list, free);
    }

    if (action != ACTION_SUPERFLUOUS_ONLY) {
	/* XXX: check error */
	r_hashtypes(db, &romhashtypes, &diskhashtypes);
	/* XXX: merge in olddb */
    }
    
    if (action != ACTION_CLEANUP_EXTRA_ONLY)
	superfluous = find_superfluous(dbname);

    if ((fix_options & (FIX_DO|FIX_PRINT))
	&& (fix_options & FIX_CLEANUP_EXTRA))
	ensure_extra_maps((action==ACTION_CHECK_ROMSET ? DO_MAP : 0)
			  | DO_LIST);

    if (action == ACTION_CHECK_ROMSET) {
	tree_traverse(tree, NULL, NULL);

	if (fix_options & (FIX_DO|FIX_PRINT)) {
	    if (needed_delete_list)
		delete_list_execute(needed_delete_list);

	    if (fix_options & FIX_SUPERFLUOUS)
		cleanup_list(superfluous, superfluous_delete_list,
			     CLEANUP_NEEDED|CLEANUP_UNKNOWN);
	    else if (superfluous_delete_list)
		delete_list_execute(superfluous_delete_list);
	}
    }

    if ((fix_options & (FIX_DO|FIX_PRINT))
	&& (fix_options & FIX_CLEANUP_EXTRA))
	cleanup_list(extra_list, extra_delete_list, 0);
    else if (extra_delete_list)
	delete_list_execute(extra_delete_list);

    if ((action == ACTION_CHECK_ROMSET &&
	 (optind == argc && (output_options & WARN_SUPERFLUOUS)))
	|| action == ACTION_SUPERFLUOUS_ONLY)
	print_superfluous(superfluous);
    
    exit(0);
}



static void
error_multiple_actions(void)
{
    fprintf(stderr,
	    "%s: only one of --cleanup-extra, --superfluous, game can be used",
	    prg);
    exit(1);
}
