/*
  mkmamedb.c -- create mamedb
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
#include <stdlib.h>

#include "config.h"

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "getopt.h"
#endif

#include "types.h"
#include "dbl.h"
#include "funcs.h"
#include "mkmamedb.h"
#include "error.h"

char *prg;
char *usage = "Usage: %s [-hV] [-o dbfile] [rominfo-file]\n";

char help_head[] = "mkmamedb (" PACKAGE ") by Dieter Baron and"
                   " Thomas Klausner\n\n";

char help[] = "\n\
  -h, --help           display this help message\n\
  -V, --version        display version number\n\
  -o, --output DBFILE  write to db DBFILE\n\
\n\
Report bugs to <nih@giga.or.at>.\n";

char version_string[] = "mkmamedb (" PACKAGE " " VERSION ")\n\
Copyright (C) 1999 Dieter Baron and Thomas Klausner\n\
" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of\n\
" PACKAGE " under the terms of the GNU General Public License.\n\
For more information about these matters, see the files named COPYING.\n";

#define OPTIONS "hVo:"

struct option options[] = {
    { "help",          0, 0, 'h' },
    { "version",       0, 0, 'V' },
    { "output",        1, 0, 'o' },
    { NULL,            0, 0, 0 },
};

int
main(int argc, char **argv)
{
    DB *db;
    char *dbname, *fname;
    int dbext;
    char c;

    prg = argv[0];

    dbname = getenv("MAMEDB");
    dbext = 0;
    if (dbname == NULL) {
	dbname = "mame";
	dbext = 1;
    }
    fname = "db.txt";
    
    optind = opterr = 0;
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
	case 'o':
	    dbname = optarg;
	    dbext = 0;
	    break;
    	default:
	    fprintf(stderr, usage, prg);
	    exit(1);
	}
    }

    switch (argc - optind) {
    case 1:
	fname = argv[optind];
	break;
    case 0:
	break;
    default:
	fprintf(stderr, usage, prg);
	exit(1);
    }
    

    if (dbext)
	dbname = ddb_name(dbname);

    remove(dbname);
    db = ddb_open(dbname, 0, 1);

    if (db==NULL) {
	myerror(ERRSTR, "can't create db '%s': %s", dbname, ddb_error());
	exit(1);
    }

    dbread_init();
    dbread(db, fname);

    ddb_close(db);

    if (dbext)
	free(dbname);

    return 0;
}
