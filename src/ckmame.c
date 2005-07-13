/*
  $NiH: ckmame.c,v 1.3 2005/07/07 22:00:20 dillo Exp $

  ckmame.c -- main routine for ckmame
  Copyright (C) 1999, 2003, 2004, 2005 Dieter Baron and Thomas Klausner

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
#include "funcs.h"
#include "tree.h"
#include "types.h"
#include "util.h"
#include "warn.h"



char *prg;

char *usage = "Usage: %s [-hVSwsfbdcFKkUuLlvn] [-D dbfile] [game...]\n";

char help_head[] = PACKAGE " by Dieter Baron and Thomas Klausner\n\n";

char help[] = "\n"
"  -b, --nobroken       don't report unfixable errors\n"
"  -c, --correct        report correct sets\n"
"  -D, --db dbfile      use mame-db dbfile\n"
"  -d, --nonogooddumps  don't report roms with no good dumps\n"
"  -F, --fix            fix rom sets\n"
"  -f, --nofixable      don't report fixable errors\n"
"  -h, --help           display this help message\n"
"  -i, --integrity      check integrity of rom files and disk images\n"
"  -K, --keep-unknown   keep unknown files when fixing (default)\n"
"  -k, --delete-unknown don't keep unknown files when fixing\n"
"  -L, --keep-long      keep long files when fixing (default)\n"
"  -l, --delete-long    don't keep long files when fixing\n"
"  -n, --dryrun         don't actually fix, only report what would be done\n"
"  -S, --samples        check samples instead of roms\n"
"      --superfuous     only check for superfluous files in rom sets\n"
"  -s, --nosuperfluous  don't report superfluous files in rom sets\n"
"  -U, --keep-unused    keep unused files when fixing\n"
"  -u, --delete-unused  don't keep unused files when fixing (default)\n"
"  -V, --version        display version number\n"
"  -v, --verbose        print fixes made\n"
"  -w, --nowarnings     print only unfixable errors\n"
"  -X, --ignoreextra    ignore extra files in rom/samples dirs\n"
"\nReport bugs to <nih@giga.or.at>.\n";

char version_string[] = PACKAGE " " VERSION "\n"
"Copyright (C) 2005 Dieter Baron and Thomas Klausner\n"
PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n"
"You may redistribute copies of\n"
PACKAGE " under the terms of the GNU General Public License.\n"
"For more information about these matters, see the files named COPYING.\n";

#define OPTIONS "bcD:dFfhiKkLlnSsxUuVvwX"

#define OPT_SF	256

struct option options[] = {
    { "help",          0, 0, 'h' },
    { "version",       0, 0, 'V' },

    { "correct",       0, 0, 'c' }, /* +CORRECT */
    { "db",            1, 0, 'D' },
    { "delete-long",   0, 0, 'l' },
    { "delete-unknown",0, 0, 'k' },
    { "delete-unused" ,0, 0, 'u' },
    { "dryrun",        0, 0, 'n' },
    { "fix",           0, 0, 'F' },
    { "ignoreextra",   0, 0, 'X' },
    { "integrity",     0, 0, 'i' },
    { "keep-long",     0, 0, 'L' },
    { "keep-unknown",  0, 0, 'K' },
    { "keep-unused",   0, 0, 'U' },
    { "nobroken",      0, 0, 'b' }, /* -BROKEN */
    { "nofixable",     0, 0, 'f' }, /* -FIX */
    { "nonogooddumps", 0, 0, 'd' }, /* -NO_GOOD_DUMPS */
    { "nosuperfluous", 0, 0, 's' }, /* -SUP */
    { "nowarnings",    0, 0, 'w' }, /* -SUP, -FIX */
    { "samples",       0, 0, 'S' },
    { "superfluous",   0, 0, OPT_SF },
    { "verbose",       0, 0, 'v' },

    { NULL,            0, 0, 0 },
};

int output_options;
int fix_do, fix_print, fix_keep_long, fix_keep_unused, fix_keep_unknown;
int ignore_extra;
int romhashtypes, diskhashtypes;



int
main(int argc, char **argv)
{
    int i, j;
    DB *db;
    char *dbname;
    int c, found;
    parray_t *list;
    tree_t *tree;
    filetype_t ft;
    int superfluous_only, integrity;
    
    prg = argv[0];
    output_options = WARN_ALL;
    ft = TYPE_ROM;
    superfluous_only = 0;
    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = DDB_DEFAULT_DB_NAME;
    fix_do = fix_print = 0;
    fix_keep_long = fix_keep_unknown = 1;
    fix_keep_unused = 0;
    ignore_extra = 0;
    integrity = 0;
    
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
	case 'F':
	    fix_do = 1;
	    break;
	case 'f':
	    output_options &= ~WARN_FIXABLE;
	    break;
	case 'i':
	    integrity = 1;
	    break;
	case 'K':
	    fix_keep_unknown = 1;
	    break;
	case 'k':
	    fix_keep_unknown = 0;
	    break;
	case 'L':
	    fix_keep_long = 1;
	    break;
	case 'l':
	    fix_keep_long = 0;
	    break;
	case 'n':
	    fix_do = 0;
	    fix_print = 1;
	    break;
	case 'S':
	    ft = TYPE_SAMPLE;
	    break;
	case 's':
	    output_options &= ~WARN_SUPERFLUOUS;
	    break;
	case 'U':
	    fix_keep_unused = 1;
	    break;
	case 'u':
	    fix_keep_unused = 0;
	    break;
	case 'v':
	    fix_print = 1;
	    break;
	case 'w':
	    output_options &= WARN_BROKEN;
	    break;
	case 'X':
	    ignore_extra = 1;
	    break;
	case OPT_SF:
	    superfluous_only = 1;
	    break;

	default:
	    fprintf(stderr, usage, prg);
	    exit(1);
	}
    }
    
    if ((db=ddb_open(dbname, DDB_READ))==NULL) {
	myerror(ERRDB, "can't open database `%s'", dbname);
	exit(1);
    }

    if (superfluous_only) {
	if (optind != argc) {
	    fprintf(stderr, usage, prg);
	    exit(1);
	}
	
	handle_extra_files(db, dbname, ft);
	exit(0);
    }

    romhashtypes = diskhashtypes = 0;
    if (integrity) {
	/* XXX: check error */
	r_hashtypes(db, &romhashtypes, &diskhashtypes);
    }
    
    if ((list=r_list(db, DDB_KEY_LIST_GAME)) == NULL) {
	myerror(ERRDEF, "list of games not found in database `%s'", dbname);
	exit(1);
    }

    tree = tree_new(ft);

    if (optind == argc) {
	for (i=0; i<parray_length(list); i++)
	    tree_add(db, tree, parray_get(list, i), ft);
    }
    else {
	for (i=optind; i<argc; i++) {
	    if (strcspn(argv[i], "*?[]{}") == strlen(argv[i])) {
		if (parray_index_sorted(list, argv[i], strcasecmp) >= 0)
		    tree_add(db, tree, argv[i], ft);
		else
		    myerror(ERRDEF, "game `%s' unknown", argv[i]);
	    }
	    else {
		found = 0;
		for (j=0; j<parray_length(list); j++) {
		    if (fnmatch(argv[i], parray_get(list, j), 0) == 0) {
			tree_add(db, tree, parray_get(list, j), ft);
			found = 1;
		    }
		}
		if (!found)
		    myerror(ERRDEF, "no game matching `%s' found", argv[i]);
	    }
	}
    }

    parray_free(list, free);

    tree_traverse(db, tree);

    if (optind == argc && !ignore_extra)
	handle_extra_files(db, dbname, ft);

    return 0;
}
