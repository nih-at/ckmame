/*
  $NiH: mkmamedb.c,v 1.10 2006/05/31 22:12:49 dillo Exp $

  mkmamedb.c -- create mamedb
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

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
#include "dbh.h"
#include "funcs.h"
#include "error.h"
#include "output.h"
#include "parse.h"
#include "w.h"
#include "xmalloc.h"

char *prg;
char *usage = "Usage: %s [-hV] [-i pat] [-o dbfile] [-F fmt] [--prog-name name] [--prog-version version] [rominfo-file ...]\n";

char help_head[] = "mkmamedb (" PACKAGE ") by Dieter Baron and"
                   " Thomas Klausner\n\n";

char help[] = "\n\
  -h, --help                display this help message\n\
  -V, --version             display version number\n\
  -F, --format [db|cm]      specify output format [default: db]\n\
  -o, --output dbfile       write to database dbfile\n\
  -x, --exclude pat         exclude games matching shell glob PAT\n\
      --prog-description d  set description of rominfo\n\
      --prog-name name      set name of program rominfo is from\n\
      --prog-version vers   set version of program rominfo is from\n\
\n\
Report bugs to " PACKAGE_BUGREPORT ".\n";

char version_string[] = "mkmamedb (" PACKAGE " " VERSION ")\n\
Copyright (C) 2006 Dieter Baron and Thomas Klausner\n\
" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of\n\
" PACKAGE " under the terms of the GNU General Public License.\n\
For more information about these matters, see the files named COPYING.\n";

#define OPTIONS "hF:o:Vx:"

enum {
    OPT_PROG_DESCRIPTION = 256,
    OPT_PROG_NAME,
    OPT_PROG_VERSION
};

struct option options[] = {
    { "help",             0, 0, 'h' },
    { "version",          0, 0, 'V' },
    { "exclude",          1, 0, 'x' },
    { "format",           1, 0, 'F' },
    { "output",           1, 0, 'o' },
    { "prog-description", 1, 0, OPT_PROG_DESCRIPTION },
    { "prog-name",        1, 0, OPT_PROG_NAME },
    { "prog-version",     1, 0, OPT_PROG_VERSION },
    { NULL,               0, 0, 0 },
};

/* XXX: needed by archive.c:read_infos_from_zip */
int romhashtypes = 0;



int
main(int argc, char **argv)
{
    output_context_t *out;
    char *dbname;
    parray_t *exclude;
    dat_entry_t dat;
    output_format_t fmt;
    int c, i;
    DB *db;

    prg = argv[0];

    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = DBH_DEFAULT_DB_NAME;
    dat_entry_init(&dat);
    exclude = NULL;
    fmt = OUTPUT_FMT_DB;

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
	case 'F':
	    if (strcmp(optarg, "cm") == 0)
		fmt = OUTPUT_FMT_CM;
	    else if (strcmp(optarg, "db") == 0)
		fmt = OUTPUT_FMT_DB;
	    else {
		fprintf(stderr, "%s: unknown output format `%s'\n",
			prg, optarg);
		exit(1);
	    }
	    break;
	case 'o':
	    dbname = optarg;
	    break;
	case 'x':
	    if (exclude == NULL)
		exclude = parray_new();
	    parray_push(exclude, xstrdup(optarg));
	    break;
	case OPT_PROG_DESCRIPTION:
	    dat_entry_description(&dat) = optarg;
	    break;
	case OPT_PROG_NAME:
	    dat_entry_name(&dat) = optarg;
	    break;
	case OPT_PROG_VERSION:
	    dat_entry_version(&dat) = optarg;
	    break;
    	default:
	    fprintf(stderr, usage, prg);
	    exit(1);
	}
    }

    if (argc - optind > 1 && dat_entry_name(&dat)) {
	fprintf(stderr,
		"%s: warning: multiple input files specified, \n\t"
		"--prog-name and --prog-version are ignored", prg);
	dat_entry_finalize(&dat);
	dat_entry_init(&dat);
    }

    if ((out=output_new(fmt, dbname)) == NULL)
	    exit(1);

    /* XXX: handle errors */
    if (optind == argc)
	parse(NULL, exclude, &dat, out);
    else {
	for (i=optind; i<argc; i++) {
	    if ((db=dbh_open(argv[i], DBL_READ)) != NULL) {
		export_db(db, exclude, &dat, out);
	    }
	    else
		parse(argv[i], exclude, &dat, out);
	}
    }

    output_close(out);

    if (exclude)
	parray_free(exclude, free);

    return 0;
}
