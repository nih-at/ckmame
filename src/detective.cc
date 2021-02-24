/*
  detective.c -- list files from zip archive with headers skipped
  Copyright (C) 2007-2014 Dieter Baron and Thomas Klausner

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

#include <cerrno>
#include <cinttypes>
#include <cstring>

#include "config.h"
#include "compat.h"
#include "error.h"
#include "globals.h"
#include "hashes.h"
#include "romdb.h"
#include "stats.h"


const char *usage = "Usage: %s [-hV] [-C types] [-D dbfile] [--detector detector] zip-archive [...]\n";

const char help_head[] = "detective (" PACKAGE ") by Dieter Baron and"
		   " Thomas Klausner\n\n";

const char help[] = "\n"
	      "  -h, --help               display this help message\n"
	      "  -V, --version            display version number\n"
	      "  -C, --hash-types types   specify hash types to compute (default: all)\n"
	      "  -D, --db dbfile          use mame-db dbfile\n"
	      "      --detector xml-file  use header detector\n"
	      "  -u, --roms-unzipped      ROMs are files on disk, not contained in zip archives\n"
	      "\n"
	      "Report bugs to " PACKAGE_BUGREPORT ".\n";

const char version_string[] = "detective (" PACKAGE " " VERSION ")\n"
			"Copyright (C) 2007-2014 Dieter Baron and Thomas Klausner\n" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

#define OPTIONS "hC:DuV"

enum { OPT_DETECTOR = 256 };

struct option options[] = {
    {"help", 0, 0, 'h'}, {"version", 0, 0, 'V'}, {"db", 1, 0, 'D'}, {"detector", 1, 0, OPT_DETECTOR}, {"hash-types", 1, 0, 'C'}, {"roms-unzipped", 0, 0, 'u'}, {NULL, 0, 0, 0},
};


static int print_archive(const char *, int);
static void print_checksums(const Hashes *, int);


int
main(int argc, char **argv) {
    const char *dbname;
    char *detector_name;
    int c, i, ret;
    int hashtypes;

    setprogname(argv[0]);

    detector = NULL;
    hashtypes = -1;

    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = DBH_DEFAULT_DB_NAME;
    detector_name = NULL;
    roms_unzipped = false;

    opterr = 0;
    while ((c = getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
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
	    hashtypes = Hashes::types_from_string(optarg);
	    if (hashtypes == 0) {
		fprintf(stderr, "%s: illegal hash types '%s'\n", getprogname(), optarg);
		exit(1);
	    }
	    break;
	case 'D':
	    dbname = optarg;
	    break;
	case 'u':
	    roms_unzipped = true;
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
#if defined(HAVE_LIBXML2)
        if ((detector = Detector::parse(detector_name)) == NULL) {
	    myerror(ERRSTR, "cannot parse detector '%s'", detector_name);
	    exit(1);
	}
#else
	myerror(ERRDEF, "mkmamedb was built without XML support, detectors not available");
#endif
    }

    try {
	auto ddb = std::make_unique<RomDB>(dbname, DBH_READ);
	if (detector == NULL) {
	    detector = ddb->read_detector();
	}
	if (hashtypes == -1) {
	    hashtypes = ddb->hashtypes(TYPE_ROM);
	}
    }
    catch (std::exception &e) {
	if (detector == 0) {
	    myerror(0, "can't open database '%s': %s", dbname, errno == EFTYPE ? "unsupported database version, please recreate" : strerror(errno) );
	    exit(1);
	}
    }

    ret = 0;
    for (i = optind; i < argc; i++)
	ret |= print_archive(argv[i], hashtypes);

    return ret ? 1 : 0;
}


static int
print_archive(const char *fname, int hashtypes) {
    int ret;

    auto archive = Archive::open(fname, TYPE_ROM, FILE_NOWHERE, ARCHIVE_FL_NOCACHE);
    if (!archive) {
	return -1;
    }

    printf("%s\n", archive->name.c_str());

    ret = 0;
    for (size_t i = 0; i < archive->files.size(); i++) {
	bool use_detector = false;
	if (!archive->file_ensure_hashes(i, hashtypes)) {
	    ret = -1;
	    continue;
	}

        auto &file = archive->files[i];

	if (file.size_hashes_are_set(true)) {
	    use_detector = true;
	}

	printf("\tfile %-12s  size %7" PRIu64, file.name.c_str(), file.get_size(use_detector));
	print_checksums(&file.get_hashes(use_detector), hashtypes);
	if (use_detector) {
	    printf("  (header skipped)");
	}
	printf("\n");
    }

    return ret;
}


static void
print_checksums(const Hashes *hashes, int hashtypes) {
    int i;

    for (i = 1; i <= Hashes::TYPE_MAX; i <<= 1) {
	if (hashes->has_type(i) && (hashtypes & i)) {
	    printf(" %s %s", Hashes::type_name(i).c_str(), hashes->to_string(i).c_str());
	}
    }
}
