/*
  $NiH$

  superflouos.c -- check for unknown file in rom directories
  Copyright (C) 1999 Dieter Baron and Thomas Klaunser

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

char help_head[] = "superflouos (" PACKAGE ") by Dieter Baron and Thomas Klausner\n\n";

char help[] = "\n\
  -h, --help           display this help message\n\
  -V, --version        display version number\n\
  -D, --db DBFILE      use mame-db DBFILE\n\
\n\
Report bugs to <nih@giga.or.at>.\n";

char version_string[] = "superflouos (" PACKAGE ") " VERSION "\n\
Copyright (C) 1999 Dieter Baron and Thomas Klausner\n\
" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of\n\
" PACKAGE " under the terms of the GNU General Public License.\n\
For more information about these matters, see the files named COPYING.\n";

#define OPTIONS "hVD:"

struct option options[] = {
    { "help",          0, 0, 'h' },
    { "version",       0, 0, 'V' },
    { "db",            1, 0, 'D' },
    { NULL,            0, 0, 0 },
};

extern char *rompath[];



int
main(int argc, char **argv)
{
    FILE *fin;
    int i;
    DB *db;
    char **list, *dbname, b[8192], *p;
    int c, nlist, dbext, l, zip;
    
    prg = argv[0];

    dbext = 0;
    dbname = getenv("MAMEDB");
    if (dbname == NULL) {
	dbname = "mame";
	dbext = 1;
    }

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

    init_rompath();

    p = b;
    for (i=0; rompath[i]; i++) {
	sprintf(b, "ls %s/roms", rompath[i]);
	if ((fin=popen(b, "r")) == NULL) {
	    /* XXX: error */
	    continue;
	}

	while (fgets(b, 8192, fin)) {
	    l = strlen(b);
	    zip = 0;
	    if (b[l-1] == '\n')
		b[--l] = '\0';
	    if (l > 4 && (strcmp(b+l-4, ".zip") == 0)) {
		zip = 1;
		b[l-4] = '\0';
	    }
	    if (bsearch(&p, list, nlist, sizeof(char *),
			(cmpfunc)strpcasecmp) == NULL)
		printf("%s/roms/%s%s\n", rompath[i], b, (zip ? ".zip" : ""));
	}
	pclose(fin);
    }

    return 0;
}
