/*
  $NiH: dumpgame.c,v 1.25 2003/03/16 10:21:33 wiz Exp $

  dumpgame.c -- print info about game (from data base)
  Copyright (C) 1999, 2003 Dieter Baron and Thomas Klausner

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
#include <fnmatch.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "getopt.h"
#endif

#include "types.h"
#include "dbh.h"
#include "error.h"
#include "util.h"
#include "romutil.h"

int dump_game(DB *db, char *name);

char *prg;
char *usage = "Usage: %s [-hV] [-D dbfile] [game ...]\n";

char help_head[] = "dumpgame (" PACKAGE ") by Dieter Baron and"
                   " Thomas Klausner\n\n";

char help[] = "\n\
  -h, --help           display this help message\n\
  -V, --version        display version number\n\
  -D, --db DBFILE      use db DBFILE\n\
\n\
Report bugs to <nih@giga.or.at>.\n";

char version_string[] = "dumpgame (" PACKAGE " " VERSION ")\n\
Copyright (C) 2003 Dieter Baron and Thomas Klausner\n\
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

static char *where_name[] = {
    "zip", "cloneof", "grand-cloneof"
};

static char *flags_name[] = {
    "ok", "baddump", "nogooddump"
};



int
main(int argc, char **argv)
{
    int i, j, nlist, found, first;
    char *dbname;
    DB *db;
    char **list;
    int dbext;
    int c;
    
    prg = argv[0];

    dbext = 0;
    dbname = getenv("MAMEDB");
    if (dbname == NULL) {
	dbname = "mame";
	dbext = DDB_EXT;
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

    if ((db=ddb_open(dbname, DDB_READ|dbext))==NULL) {
	myerror(ERRSTR, "can't open database `%s'", dbname);
	exit (1);
    }

    if ((nlist=r_list(db, "/list", &list)) < 0) {
	myerror(ERRDEF, "list of games not found in database '%s'", dbname);
	exit(1);
    }

    first = 1;
    for (i=optind; i<argc; i++) {
	if (strcspn(argv[i], "*?[]{}") == strlen(argv[i])) {
	    if (bsearch(argv+i, list, nlist, sizeof(char *),
			(cmpfunc)strpcasecmp) != NULL) {
		if (first)
		    first = 0;
		else
		    putc('\n', stdout);
		dump_game(db, argv[i]);
	    }
	    else
		myerror(ERRDEF, "game `%s' unknown", argv[i]);
	}
	else {
	    found = 0;
	    for (j=0; j<nlist; j++) {
		if (fnmatch(argv[i], list[j], 0) == 0) {
		    if (first)
			first = 0;
		    else
			putc('\n', stdout);
		    dump_game(db, list[j]);
		    found = 1;
		}
	    }
	    if (!found)
		myerror(ERRDEF, "no game matching `%s' found", argv[i]);
	}
    }


    return 0;
}



int
dump_game(DB *db, char *name)
{
    int i, j;
    struct game *game;

    if ((game=r_game(db, name)) == NULL) {
	myerror(ERRDEF, "game unknown (or db error): %s", name);
	return -1;
    }
    
    printf("Name:\t\t%s\n", game->name);
    printf("Description:\t%s\n", game->description);
    if (game->cloneof[0])
	printf("Cloneof:\t%s\n", game->cloneof[0]);
    if (game->cloneof[1])
	printf("Grand-Cloneof:\t%s\n", game->cloneof[1]);
    if (game->nclone) {
	printf("Clones:");
	for (i=0; i<game->nclone; i++) {
	    if (i%6 == 0)
		fputs("\t\t", stdout);
	    printf("%-8s ", game->clone[i]);
	    if (i%6 == 5)
		putc('\n', stdout);
	}
	if (game->nclone % 6 != 0)
	    putc('\n', stdout);
    }
    printf("Roms:");
    for (i=0; i<game->nrom; i++) {
	printf("\t\tfile %-12s  size %7ld  crc %.8lx  flags %s  in %s",
	       game->rom[i].name, game->rom[i].size, game->rom[i].crc,
	       flags_name[game->rom[i].flags], where_name[game->rom[i].where]);
	if (game->rom[i].merge
	    && strcmp(game->rom[i].name, game->rom[i].merge) != 0)
	    printf(" (%s)", game->rom[i].merge);
	putc('\n', stdout);
	for (j=0; j < game->rom[i].naltname; j++) {
	    printf("\t\tfile %-12s  size %7ld  crc %.8lx  flags %s  in %s",
		   game->rom[i].altname[j], game->rom[i].size,
		   game->rom[i].crc, flags_name[game->rom[i].flags],
		   where_name[game->rom[i].where]);
	    if (game->rom[i].merge) {
		if (strcmp(game->rom[i].altname[j], game->rom[i].merge) != 0)
		    printf(" (%s)", game->rom[i].merge);
	    } else
		printf(" (%s)", game->rom[i].name);
	    putc('\n', stdout);
	}
    }
    if (game->sampleof[0])
	printf("Sampleof:\t%s\n", game->sampleof[0]);
    if (game->sampleof[1])
	printf("Grand-Sampleof:\t%s\n", game->sampleof[1]);
    if (game->nsclone) {
	printf("Sample Clones:");
	for (i=0; i<game->nsclone; i++) {
	    if (i%6 == 0)
		printf("%s\t", (i==0 ? "" : "\t"));
	    printf("%-8s ", game->sclone[i]);
	    if (i%6 == 5)
		putc('\n', stdout);
	}
	if (game->nsclone % 6 != 5)
	    putc('\n', stdout);
    }
    if (game->nsample) {
	printf("Samples:");
	for (i=0; i<game->nsample; i++)
	    printf("\t%s%-12s  in %s\n",
		   (i==0 ? "" : "\t"),
		   game->sample[i].name,
		   where_name[game->sample[i].where]);
    }

    game_free(game, 1);

    return 0;
}
