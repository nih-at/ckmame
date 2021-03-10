/*
  mkmamedb.c -- create mamedb
  Copyright (C) 1999-2020 Dieter Baron and Thomas Klausner

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

#include "config.h"
#include "compat.h"

#include <cstring>
#include <filesystem>
#include <zip.h>

#include "Archive.h"
#include "dbh_cache.h"
#include "error.h"
#include "file_util.h"
#include "globals.h"
#include "parse.h"
#include "ParserSourceFile.h"
#include "ParserSourceZip.h"
#include "RomDB.h"


const char *usage = "Usage: %s [-htuV] [-C types] [-F fmt] [-o dbfile] [-x pat] [--detector xml-file] [--no-directory-cache] [--only-files pat] [--prog-description d] [--prog-name name] [--prog-version version] [--skip-files pat] [rominfo-file ...]\n";

const char help_head[] = "mkmamedb (" PACKAGE ") by Dieter Baron and"
		   " Thomas Klausner\n\n";

const char help[] = "\n"
	      "  -h, --help                      display this help message\n"
	      "  -V, --version                   display version number\n"
	      "  -C, --hash-types types          specify hash types to compute (default: all)\n"
	      "  -F, --format [cm|dat|db|mtree]  specify output format [default: db]\n"
	      "  -o, --output dbfile             write to database dbfile\n"
	      "  -t, --use-temp-directory        create output in temporary directory, move when done\n"
	      "  -u, --roms-unzipped             ROMs are files on disk, not contained in zip archives\n"
	      "  -x, --exclude pat               exclude games matching shell glob PAT\n"
	      "      --detector xml-file         use header detector\n"
	      "      --no-directory-cache        don't create cache of scanned input directory\n"
	      "      --only-files pat            only use zip members matching shell glob PAT\n"
	      "      --prog-description d        set description of rominfo\n"
	      "      --prog-name name            set name of program rominfo is from\n"
	      "      --prog-version version      set version of program rominfo is from\n"
	      "      --runtest                   output special format for use in ckmame test suite\n"
	      "      --skip-files pat            don't use zip members matching shell glob PAT\n"
	      "\n"
	      "Report bugs to " PACKAGE_BUGREPORT ".\n";

char version_string[] = "mkmamedb (" PACKAGE " " VERSION ")\n"
			"Copyright (C) 1999-2020 Dieter Baron and Thomas Klausner\n" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

#define OPTIONS "hC:F:o:tuVx:"

enum { OPT_DETECTOR = 256, OPT_NO_DIRECTORY_CACHE, OPT_ONLY_FILES, OPT_PROG_DESCRIPTION, OPT_PROG_NAME, OPT_PROG_VERSION, OPT_RUNTEST, OPT_SKIP_FILES };

struct option options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'V'},
    {"no-directory-cache", 0, 0, OPT_NO_DIRECTORY_CACHE},
    {"detector", 1, 0, OPT_DETECTOR},
    {"exclude", 1, 0, 'x'},
    {"format", 1, 0, 'F'},
    {"hash-types", 1, 0, 'C'},
    {"output", 1, 0, 'o'},
    {"only-files", 1, 0, OPT_ONLY_FILES},
    {"prog-description", 1, 0, OPT_PROG_DESCRIPTION},
    {"prog-name", 1, 0, OPT_PROG_NAME},
    {"prog-version", 1, 0, OPT_PROG_VERSION},
    {"roms-unzipped", 0, 0, 'u'},
    {"runtest", 0, 0, OPT_RUNTEST},
    {"skip-files", 1, 0, OPT_SKIP_FILES},
    {"use-temp-directory", 0, 0, 't'},
    {NULL, 0, 0, 0},
};

#define DEFAULT_FILE_PATTERNS "*.dat"

static int process_file(const char *fname, const std::unordered_set<std::string> &exclude, const DatEntry *dat, const std::vector<std::string> &file_patterns, const std::unordered_set<std::string> &files_skip, OutputContext *out, int flags);
static int process_stdin(const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *out);

static int hashtypes;
static bool cache_directory;
static int parser_flags;

int
main(int argc, char **argv) {
    OutputContextPtr out;
    const char *dbname, *dbname_real;
    char tmpnam_buffer[L_tmpnam];
    std::unordered_set<std::string> exclude;
    std::vector<std::string> file_patterns;
    std::unordered_set<std::string> skip_files;
    DatEntry dat;
    OutputContext::Format fmt;
    char *detector_name;
    int c, i;
    int flags;
    bool runtest;
    int ret = 0;

    setprogname(argv[0]);

    runtest = false;
    detector = NULL;
    roms_unzipped = false;
    cache_directory = true;
    flags = 0;
    parser_flags = 0;

    dbname_real = NULL;
    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = DBH_DEFAULT_DB_NAME;
    fmt = OutputContext::FORMAT_DB;
    hashtypes = Hashes::TYPE_CRC | Hashes::TYPE_MD5 | Hashes::TYPE_SHA1;
    detector_name = NULL;

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
            case 'F':
                if (strcmp(optarg, "cm") == 0) {
                    fmt = OutputContext::FORMAT_CM;
                }
                else if (strcmp(optarg, "dat") == 0) {
                    fmt = OutputContext::FORMAT_DATAFILE_XML;
                }
                else if (strcmp(optarg, "db") == 0) {
                    fmt = OutputContext::FORMAT_DB;
                }
                else if (strcmp(optarg, "mtree") == 0) {
                    fmt = OutputContext::FORMAT_MTREE;
                }
                else {
                    fprintf(stderr, "%s: unknown output format '%s'\n", getprogname(), optarg);
                    exit(1);
                }
                break;
            case 'o':
                dbname = optarg;
                break;
            case 't':
                flags |= OUTPUT_FL_TEMP;
                break;
            case 'u':
                roms_unzipped = true;
                break;
            case 'x':
                exclude.insert(optarg);
                break;
            case OPT_DETECTOR:
                detector_name = optarg;
                break;
            case OPT_NO_DIRECTORY_CACHE:
                cache_directory = false;
                break;
            case OPT_ONLY_FILES:
                file_patterns.push_back(optarg);
                break;
            case OPT_PROG_DESCRIPTION:
                dat.description = optarg;
                break;
            case OPT_PROG_NAME:
                dat.name = optarg;
                break;
            case OPT_RUNTEST:
                runtest = true;
                break;
            case OPT_PROG_VERSION:
                dat.version = optarg;
                break;
            case OPT_SKIP_FILES:
                skip_files.insert(optarg);
                break;
            default:
                fprintf(stderr, usage, getprogname());
                exit(1);
        }
    }

    if (argc - optind > 1 && !dat.name.empty()) {
	fprintf(stderr,
		"%s: warning: multiple input files specified, \n\t"
		"--prog-name and --prog-version are ignored",
		getprogname());
    }
    if (runtest) {
	fmt = OutputContext::FORMAT_MTREE;
	flags |= OUTPUT_FL_RUNTEST;
	parser_flags = PARSER_FL_FULL_ARCHIVE_NAME;
	cache_directory = false;
    }

    if (flags & OUTPUT_FL_TEMP) {
	dbname_real = dbname;
	dbname = tmpnam(tmpnam_buffer);
	if (dbname == NULL) {
	    myerror(ERRSTR, "tmpnam() failed");
	    exit(1);
	}
    }

    if ((out = OutputContext::create(fmt, dbname, flags)) == NULL) {
	exit(1);
    }

    if (detector_name) {
#if defined(HAVE_LIBXML2)
	seterrinfo(detector_name);
	detector = Detector::parse(detector_name);
        if (detector != NULL) {
            out->detector(detector.get());
        }
#else
	myerror(ERRDEF, "mkmamedb was built without XML support, detectors not available");
#endif
    }


    /* TODO: handle errors */
    if (optind == argc) {
	process_stdin(exclude, &dat, out.get());
    }
    else {
        // TODO: this isn't overridable by --only-files?
        file_patterns.push_back(DEFAULT_FILE_PATTERNS);

	for (i = optind; i < argc; i++) {
	    if (process_file(argv[i], exclude, &dat, file_patterns, skip_files, out.get(), flags) < 0) {
		i = argc;
		ret = -1;
	    }
	}
    }

    if (ret == 0) {
	out->close();
    }

    if (roms_unzipped) {
	dbh_cache_close_all();
    }

    if (flags & OUTPUT_FL_TEMP) {
	if (!rename_or_move(dbname, dbname_real)) {
	    myerror(ERRDEF, "could not copy temporary output '%s' to '%s'", dbname, dbname_real);
	    return 1;
	}
    }
    
    return 0;
}


static int process_file(const char *fname, const std::unordered_set<std::string> &exclude, const DatEntry *dat, const std::vector<std::string> &file_patterns, const std::unordered_set<std::string> &files_skip, OutputContext *out, int flags) {
    struct zip *za;

    try {
	auto mdb = std::make_unique<RomDB>(fname, DBH_READ);
	return mdb->export_db(exclude, dat, out);
    }
    catch (std::exception &e) {
	/* that's fine */
    }

    if (!roms_unzipped && (za = zip_open(fname, 0, NULL)) != NULL) {
	const char *name;
	int err;

	err = 0;
	for (uint64_t i = 0; i < zip_get_num_entries(za, 0); i++) {
	    name = zip_get_name(za, i, 0);

            if (files_skip.find(name) != files_skip.end()) {
                continue;
            }
            auto skip = true;
            for (auto &pattern : file_patterns) {
                if (fnmatch(pattern.c_str(), name, 0) == 0) {
                    skip = false;
                    break;
                }
            }
            if (skip) {
                continue;
            }
            try {
                auto ps = std::make_shared<ParserSourceZip>(fname, za, name);

                if (!ParserContext::parse(ps, exclude, dat, out, parser_flags)) {
                    err = -1;
                }
           }
            catch (std::exception &e) {
		err = -1;
		continue;
	    }
 	}
	zip_close(za);

	return err;
    }
    else {
	std::error_code ec;
	if (std::filesystem::is_directory(fname, ec)) {
            if (cache_directory) {
		Archive::register_cache_directory(fname);
            }

            auto ctx = ParserContext(NULL, exclude, dat, out, parser_flags);
            return ctx.parse_dir(fname, hashtypes, flags & OUTPUT_FL_RUNTEST);
	}

	if (ec) {
	    myerror(ERRDEF, "cannot stat() file '%s': %s", fname, ec.message().c_str());
	    return -1;
	}

	if (roms_unzipped) {
	    myerror(ERRDEF, "argument '%s' is not a directory", fname);
	    return -1;
	}

        try {
            auto ps = std::make_shared<ParserSourceFile>(fname);
            return ParserContext::parse(ps, exclude, dat, out, parser_flags);
        }
        catch (std::exception &e) {
	    return -1;
        }
    }
}


static int process_stdin(const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *out) {
    try {
        auto ps = std::make_shared<ParserSourceFile>("");

        return ParserContext::parse(ps, exclude, dat, out, parser_flags);
    }
    catch (std::exception &e) {
        return -1;
    }
}
