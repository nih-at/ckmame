/*
  $NiH: ckmame.c,v 1.26 2002/06/06 09:26:50 dillo Exp $

  ckmame.c -- main routine for ckmame
  Copyright (C) 1999, 2003 Dieter Baron and Thomas Klaunser

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

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

#include "types.h"
#include "dbh.h"
#include "util.h"
#include "funcs.h"
#include "error.h"



char *prg;

char *usage = "Usage: %s [-hVSwsfbdcFKkUuLlvn] [-D dbfile] [game...]\n";

char help_head[] = PACKAGE " by Dieter Baron and Thomas Klausner\n\n";

char help[] = "\n\
  -h, --help           display this help message\n\
  -V, --version        display version number\n\
  -D, --db DBFILE      use mame-db DBFILE\n\
  -S, --samples        check samples instead of roms\n\
  -w, --nowarnings     print only unfixable errors\n\
  -s, --nosuperfluous  don't report superfluous files\n\
  -f, --nofixable      don't report fixable errors\n\
  -b, --nobroken       don't report unfixable errors\n\
  -d, --nonogooddumps  don't report roms with no good dumps\n\
  -c, --correct        report correct sets\n\
  -F, --fix            fix rom sets\n\
  -K, --keep-unknown   keep unknown files when fixing (default)\n\
  -k, --delete-unknown don't keep unknown files when fixing\n\
  -U, --keep-unused    keep unused files when fixing\n\
  -u, --delete-unused  don't keep unused files when fixing (default)\n\
  -L, --keep-long      keep long files when fixing (default)\n\
  -l, --delete-long    don't keep long files when fixing\n\
  -v, --verbose        print fixes made\n\
  -n, --dryrun         don't actually fix, only report what would be done\n\
\n\
Report bugs to <nih@giga.or.at>.\n";

char version_string[] = PACKAGE " " VERSION "\n\
Copyright (C) 2003 Dieter Baron and Thomas Klausner\n\
" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of\n\
" PACKAGE " under the terms of the GNU General Public License.\n\
For more information about these matters, see the files named COPYING.\n";

#define OPTIONS "hVD:SwsfbcdxFKkUuLlvn"

struct option options[] = {
    { "help",          0, 0, 'h' },
    { "version",       0, 0, 'V' },
    { "db",            1, 0, 'D' },
    { "samples",       0, 0, 'S' },
    { "nowarnings",    0, 0, 'w' }, /* -SUP, -FIX */
    { "nosuperfluous", 0, 0, 's' }, /* -SUP */
    { "nofixable",     0, 0, 'f' }, /* -FIX */
    { "nobroken",      0, 0, 'b' }, /* -BROKEN */
    { "nonogooddumps", 0, 0, 'd' }, /* -NO_GOOD_DUMPS */
    { "correct",       0, 0, 'c' }, /* +CORRECT */
    { "fix",           0, 0, 'F' },
    { "keep-unknown",  0, 0, 'K' },
    { "delete-unknown",0, 0, 'k' },
    { "keep-unused",   0, 0, 'U' },
    { "delete-unused" ,0, 0, 'u' },
    { "keep-long",     0, 0, 'L' },
    { "delete-long",   0, 0, 'l' },
    { "verbose",       0, 0, 'v' },
    { "dryrun",        0, 0, 'n' },
    { NULL,            0, 0, 0 },
};

int output_options;
int fix_do, fix_print, fix_keep_long, fix_keep_unused, fix_keep_unknown;



int
main(int argc, char **argv)
{
    int i, j;
    DB *db;
    char **list, *dbname;
    int c, nlist, found, dbext;
    struct tree *tree;
    struct tree tree_root;
    int sample;
    
    prg = argv[0];
    tree = &tree_root;
    tree->child = NULL;
    output_options = WARN_ALL;
    sample = 0;
    dbext = 0;
    dbname = getenv("MAMEDB");
    if (dbname == NULL) {
	dbname = "mame";
	dbext = 1;
    }
    fix_do = fix_print = 0;
    fix_keep_long = fix_keep_unknown = 1;
    fix_keep_unused = 0;

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
	case 'D':
	    dbname = optarg;
	    dbext = 0;
	    break;
	case 'S':
	    sample = 1;
	    break;
	case 'w':
	    output_options &= WARN_BROKEN;
	    break;
	case 's':
	    output_options &= ~WARN_SUPERFLUOUS;
	    break;
	case 'f':
	    output_options &= ~WARN_FIXABLE;
	    break;
	case 'b':
	    output_options &= ~WARN_BROKEN;
	    break;
	case 'c':
	    output_options |= WARN_CORRECT;
	    break;
	case 'd':
	    output_options &= ~WARN_NO_GOOD_DUMP;
	    break;
	case 'F':
	    fix_do = 1;
	    break;
	case 'K':
	    fix_keep_unknown = 1;
	    break;
	case 'k':
	    fix_keep_unknown = 0;
	    break;
	case 'U':
	    fix_keep_unused = 1;
	    break;
	case 'u':
	    fix_keep_unused = 0;
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
	case 'v':
	    fix_print = 1;
	    break;
	default:
	    fprintf(stderr, usage, prg);
	    exit(1);
	}
    }
    
    if ((db=ddb_open(dbname, dbext, 0))==NULL) {
	myerror(ERRSTR, "can't open database `%s'", dbname);
	exit(1);
    }

    if ((nlist=r_list(db, "/list", &list)) < 0) {
	myerror(ERRDEF, "list of games not found in database `%s'", dbname);
	exit(1);
    }

    if (optind == argc) {
	for (i=0; i<nlist; i++)
	    tree_add(db, tree, list[i], sample);
    }
    else {
	for (i=optind; i<argc; i++) {
	    if (strcspn(argv[i], "*?[]{}") == strlen(argv[i])) {
		if (bsearch(argv+i, list, nlist, sizeof(char *),
			    (cmpfunc)strpcasecmp) != NULL)
		    tree_add(db, tree, argv[i], sample);
		else
		    myerror(ERRDEF, "game `%s' unknown", argv[i]);
	    }
	    else {
		found = 0;
		for (j=0; j<nlist; j++) {
		    if (fnmatch(argv[i], list[j], 0) == 0) {
			tree_add(db, tree, list[j], sample);
			found = 1;
		    }
		}
		if (!found)
		    myerror(ERRDEF, "no game matching `%s' found", argv[i]);
	    }
	}
    }

    tree_traverse(db, tree, sample);

    return 0;
}
