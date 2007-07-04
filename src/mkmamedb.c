/*
  $NiH: mkmamedb.c,v 1.14 2007/04/10 23:01:30 dillo Exp $

  mkmamedb.c -- create mamedb
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

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



#include <stdio.h>
#include <stdlib.h>

#include "compat.h"
#include "dbh.h"
#include "funcs.h"
#include "error.h"
#include "output.h"
#include "parse.h"
#include "types.h"
#include "xmalloc.h"

char *usage = "Usage: %s [-hV] [-C types] [-i pat] [-o dbfile] [-F fmt] [--prog-name name] [--prog-version version] [rominfo-file ...]\n";

char help_head[] = "mkmamedb (" PACKAGE ") by Dieter Baron and"
                   " Thomas Klausner\n\n";

char help[] = "\n\
  -h, --help                display this help message\n\
  -V, --version             display version number\n\
  -C, --hash-types types    specify hash types to compute (default: all)\n\
  -F, --format [db|cm]      specify output format [default: db]\n\
  -o, --output dbfile       write to database dbfile\n\
  -x, --exclude pat         exclude games matching shell glob PAT\n\
      --detector xml-file   use header detector\n\
      --prog-description d  set description of rominfo\n\
      --prog-name name      set name of program rominfo is from\n\
      --prog-version vers   set version of program rominfo is from\n\
\n\
Report bugs to " PACKAGE_BUGREPORT ".\n";

char version_string[] = "mkmamedb (" PACKAGE " " VERSION ")\n\
Copyright (C) 2007 Dieter Baron and Thomas Klausner\n\
" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

#define OPTIONS "hC:F:o:Vx:"

enum {
    OPT_PROG_DESCRIPTION = 256,
    OPT_PROG_NAME,
    OPT_PROG_VERSION,
    OPT_DETECTOR
};

struct option options[] = {
    { "help",             0, 0, 'h' },
    { "version",          0, 0, 'V' },
    { "detector",         1, 0, OPT_DETECTOR },
    { "exclude",          1, 0, 'x' },
    { "format",           1, 0, 'F' },
    { "hash-types",       1, 0, 'C' },
    { "output",           1, 0, 'o' },
    { "prog-description", 1, 0, OPT_PROG_DESCRIPTION },
    { "prog-name",        1, 0, OPT_PROG_NAME },
    { "prog-version",     1, 0, OPT_PROG_VERSION },
    { NULL,               0, 0, 0 },
};

int romhashtypes;
detector_t *detector;



int
main(int argc, char **argv)
{
    output_context_t *out;
    char *dbname;
    parray_t *exclude;
    dat_entry_t dat;
    output_format_t fmt;
    char *detector_name;
    int c, i;
    sqlite3 *db;

    setprogname(argv[0]);

    detector = NULL;

    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = DBH_DEFAULT_DB_NAME;
    dat_entry_init(&dat);
    exclude = NULL;
    fmt = OUTPUT_FMT_DB;
    romhashtypes = HASHES_TYPE_CRC|HASHES_TYPE_MD5|HASHES_TYPE_SHA1;
    detector_name = NULL;

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
	case 'C':
	    romhashtypes=hash_types_from_str(optarg);
	    if (romhashtypes == 0) {
		fprintf(stderr, "%s: illegal hash types `%s'\n",
			getprogname(), optarg);
		exit(1);
	    }
	    break;
	case 'F':
	    if (strcmp(optarg, "cm") == 0)
		fmt = OUTPUT_FMT_CM;
	    else if (strcmp(optarg, "db") == 0)
		fmt = OUTPUT_FMT_DB;
	    else {
		fprintf(stderr, "%s: unknown output format `%s'\n",
			getprogname(), optarg);
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
	case OPT_DETECTOR:
	    detector_name = optarg;
	    break;
    	default:
	    fprintf(stderr, usage, getprogname());
	    exit(1);
	}
    }

    if (argc - optind > 1 && dat_entry_name(&dat)) {
	fprintf(stderr,
		"%s: warning: multiple input files specified, \n\t"
		"--prog-name and --prog-version are ignored", getprogname());
	dat_entry_finalize(&dat);
	dat_entry_init(&dat);
    }

    if ((out=output_new(fmt, dbname)) == NULL)
	    exit(1);

    if (detector_name) {
	seterrinfo(detector_name, NULL);
	detector = detector_parse(detector_name);
	if (detector != NULL)
	    output_detector(out, detector);
    }


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

    if (detector)
	detector_free(detector);

    if (exclude)
	parray_free(exclude, free);

    return 0;
}
