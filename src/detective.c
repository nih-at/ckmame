/*
  $NiH$

  detective.c -- list files from zip archive with headers skipped
  Copyright (C) 2007 Dieter Baron and Thomas Klausner

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

#include "archive.h"
#include "dbh.h"
#include "detector.h"
#include "error.h"
#include "globals.h"
#include "hashes.h"

char *usage = "Usage: %s [-hV] [-C types] [-D dbfile] [--detector detector] zip-archive [...]\n";

char help_head[] = "detective (" PACKAGE ") by Dieter Baron and"
                   " Thomas Klausner\n\n";

char help[] = "\n\
  -h, --help               display this help message\n\
  -V, --version            display version number\n\
  -C, --hash-types types   specify hash types to compute (default: all)\n\
  -D, --db dbfile          use mame-db dbfile\n\
      --detector xml-file  use header detector\n\
\n\
Report bugs to " PACKAGE_BUGREPORT ".\n";

char version_string[] = "detective (" PACKAGE " " VERSION ")\n\
Copyright (C) 2007 Dieter Baron and Thomas Klausner\n\
" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of\n\
" PACKAGE " under the terms of the GNU General Public License.\n\
For more information about these matters, see the files named COPYING.\n";

#define OPTIONS "hC:DV"

enum {
    OPT_DETECTOR = 256
};

struct option options[] = {
    { "help",             0, 0, 'h' },
    { "version",          0, 0, 'V' },
    { "db",               1, 0, 'D' },
    { "detector",         1, 0, OPT_DETECTOR },
    { "hash-types",       1, 0, 'C' },
    { NULL,               0, 0, 0 },
};

int romhashtypes;
detector_t *detector;



static int print_archive(const char *);
static void print_checksums(hashes_t *);



int
main(int argc, char **argv)
{
    char *dbname;
    char *detector_name;
    int c, i, ret;
    sqlite3 *db;

    setprogname(argv[0]);

    detector = NULL;

    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = DBH_DEFAULT_DB_NAME;
    romhashtypes = 0;
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
	case 'D':
	    dbname = optarg;
	    break;
	case OPT_DETECTOR:
	    detector_name = optarg;
	    break;
    	default:
	    fprintf(stderr, usage, getprogname());
	    exit(1);
	}
    }

    if (argc == optind) {
	fprintf(stderr, usage, getprogname());
	exit(1);
    }

    if (detector_name) {
	if ((detector=detector_parse(detector_name)) == NULL) {
	    myerror(ERRSTR, "cannot parse detector `%s'", detector_name);
	    exit(1);
	}
    }

    if ((db=dbh_open(dbname, DBL_READ)) == NULL) {
	if (detector == 0) {
	    myerror(ERRDB, "can't open database `%s'", dbname);
	    exit(1);
	}
	if (romhashtypes == 0)
	    romhashtypes = HASHES_TYPE_CRC|HASHES_TYPE_MD5|HASHES_TYPE_SHA1;
    }
    else {
	if (detector == NULL)
	    detector = r_detector(db);
	if (romhashtypes == 0)
	    r_hashtypes(db, &romhashtypes, &i);
	dbh_close(db);
    }

    ret = 0;
    for (i=optind; i<argc; i++)
	ret |= print_archive(argv[i]);

    return ret ? 1 : 0;
}



static int
print_archive(const char *fname)
{
    archive_t *a;
    rom_t *f;
    int i, ret;

    if ((a=archive_new(fname, 0)) == NULL)
	return -1;

    printf("%s\n", archive_name(a));

    ret = 0;
    for (i=0; i<archive_num_files(a); i++) {
	if (archive_file_compute_hashes(a, i, romhashtypes) < 0) {
	    ret = -1;
	    continue;
	}

	f = archive_file(a, i);

	printf("\tfile %-12s  size %7ld",
	       rom_name(f), rom_size(f));
	print_checksums(rom_hashes(f));
	printf("\n");
	
    }

    return ret;
}



static void
print_checksums(hashes_t *hashes)
{
    int i;
    char h[HASHES_SIZE_MAX*2 + 1];

    for (i=1; i<=HASHES_TYPE_MAX; i<<=1) {
	if (hashes_has_type(hashes, i) && (romhashtypes & i)) {
	    printf(" %s %s", hash_type_string(i),
		   hash_to_string(h, i, hashes));
	}
    }
}
