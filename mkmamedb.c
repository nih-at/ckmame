/*
  $NiH: mkmamedb.c,v 1.28 2005/06/20 16:16:04 wiz Exp $

  mkmamedb.c -- create mamedb
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
#include "error.h"
#include "w.h"
#include "parse.h"

char *prg;
char *usage = "Usage: %s [-hV] [-o dbfile] [--prog-name name] [--prog-version version] [rominfo-file]\n";

char help_head[] = "mkmamedb (" PACKAGE ") by Dieter Baron and"
                   " Thomas Klausner\n\n";

char help[] = "\n\
  -h, --help               display this help message\n\
  -V, --version            display version number\n\
  -o, --output dbfile      write to database dbfile\n\
      --prog-name name     set name of program rominfo is from\n\
      --prog-version vers  set version of program rominfo is from\n\
\n\
Report bugs to <nih@giga.or.at>.\n";

char version_string[] = "mkmamedb (" PACKAGE " " VERSION ")\n\
Copyright (C) 2005 Dieter Baron and Thomas Klausner\n\
" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of\n\
" PACKAGE " under the terms of the GNU General Public License.\n\
For more information about these matters, see the files named COPYING.\n";

#define OPTIONS "hVo:"

enum {
    OPT_PROG_NAME = 256,
    OPT_PROG_VERSION
};

struct option options[] = {
    { "help",          0, 0, 'h' },
    { "version",       0, 0, 'V' },
    { "output",        1, 0, 'o' },
    { "prog-name",     1, 0, OPT_PROG_NAME },
    { "prog-version",  1, 0, OPT_PROG_VERSION },
    { NULL,            0, 0, 0 },
};

int
main(int argc, char **argv)
{
    DB *db;
    char *dbname, *fname, *prog_name, *prog_version;
    int dbext;
    int c;

    prg = argv[0];

    dbname = getenv("MAMEDB");
    dbext = 0;
    if (dbname == NULL) {
	dbname = "mame";
	dbext = 1;
    }
    fname = NULL;

    prog_name = prog_version = NULL;

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
	case 'o':
	    dbname = optarg;
	    dbext = 0;
	    break;
	case OPT_PROG_NAME:
	    prog_name = optarg;
	    break;
	case OPT_PROG_VERSION:
	    prog_version = optarg;
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
    db = ddb_open(dbname, DDB_WRITE);

    if (db==NULL) {
	myerror(ERRDB, "can't create database '%s'", dbname);
	exit(1);
    }

    w_version(db);
    parse(db, fname, prog_name, prog_version);

    ddb_close(db);

    if (dbext)
	free(dbname);

    return 0;
}
