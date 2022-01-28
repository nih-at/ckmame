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

#include <filesystem>
#include <zip.h>

#include "Archive.h"
#include "CkmameDB.h"
#include "Commandline.h"
#include "error.h"
#include "Exception.h"
#include "file_util.h"
#include "globals.h"
#include "Parser.h"
#include "ParserDir.h"
#include "ParserSourceFile.h"
#include "ParserSourceZip.h"
#include "RomDB.h"

const char help_head[] = "mkmamedb (" PACKAGE ") by Dieter Baron and Thomas Klausner";
const char help_footer[] = "Report bugs to " PACKAGE_BUGREPORT ".";

char version_string[] = "mkmamedb (" PACKAGE " " VERSION ")\n"
			"Copyright (C) 1999-2022 Dieter Baron and Thomas Klausner\n" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

std::vector<Commandline::Option> options = {
    Commandline::Option("detector", "xml-file", "use header detector"),
    Commandline::Option("directory-cache", "create cache of scanned input directory (default)"),
    Commandline::Option("exclude", 'x', "pattern", "exclude games matching shell glob pattern"),
    Commandline::Option("format", 'F', "format", "specify output format (default: db)"),
    Commandline::Option("hash-types", 'C', "types", "specify hash types to compute (default: all)"),
    Commandline::Option("no-directory-cache", "don't create cache of scanned input directory"),
    Commandline::Option("only-files", "pattern", "only use zip members matching shell glob pattern"),
    Commandline::Option("output", 'o', "dbfile", "write to database dbfile (default: mame.db)"),
    Commandline::Option("prog-description", "description", "set description of rominfo"),
    Commandline::Option("prog-name", "name", "set name of rominfo"),
    Commandline::Option("prog-version", "version", "set version of rominfo"),
    Commandline::Option("runtest", "output special format for use in ckmame test suite"),
    Commandline::Option("skip-files", "pattern", "don't use zip members matching shell glob pattern"),
    Commandline::Option("use-temp-directory", 't', "create output in temporary directory, move when done")
};

std::unordered_set<std::string> used_variables = {
    "roms_zipped"
};

#define DEFAULT_FILE_PATTERNS "*.dat"

static bool process_file(const std::string &fname, const std::unordered_set<std::string> &exclude, const DatEntry *dat, const std::vector<std::string> &file_patterns, const std::unordered_set<std::string> &files_skip, OutputContext *out, int flags);
static bool process_stdin(const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *out);

static int hashtypes;
static bool cache_directory;
static int parser_flags;

int
main(int argc, char **argv) {
    OutputContextPtr out;
    std::string dbname, dbname_real;
    char tmpnam_buffer[L_tmpnam];
    std::unordered_set<std::string> exclude;
    std::vector<std::string> file_patterns;
    std::unordered_set<std::string> skip_files;
    DatEntry dat;
    OutputContext::Format fmt;
    std::string detector_name;
    int flags;
    bool runtest;
    int ret = 0;

    setprogname(argv[0]);

    runtest = false;
    cache_directory = true;
    flags = 0;
    parser_flags = 0;

    fmt = OutputContext::FORMAT_DB;
    hashtypes = Hashes::TYPE_CRC | Hashes::TYPE_MD5 | Hashes::TYPE_SHA1;

    std::vector<std::string> arguments;
    auto commandline = Commandline(options, "[rominfo-file ...]", help_head, help_footer, version_string);

    Configuration::add_options(commandline, used_variables);

    try {
	auto args = commandline.parse(argc, argv);

	configuration.handle_commandline(args);

        for (const auto &option : args.options) {
            if (option.name == "detector") {
                detector_name = option.argument;
            }
            else if (option.name == "directory-cache") {
                cache_directory = true;
            }
            else if (option.name == "exclude") {
                exclude.insert(option.argument);
            }
            else if (option.name == "format") {
                if (option.argument == "cm") {
                    fmt = OutputContext::FORMAT_CM;
                }
                else if (option.argument == "dat") {
                    fmt = OutputContext::FORMAT_DATAFILE_XML;
                }
                else if (option.argument == "db") {
                    fmt = OutputContext::FORMAT_DB;
                }
                else if (option.argument == "mtree") {
                    fmt = OutputContext::FORMAT_MTREE;
                }
                else {
                    fprintf(stderr, "%s: unknown output format '%s'\n", getprogname(), option.argument.c_str());
                    exit(1);
                }
            }
            else if (option.name == "hash-types") {
                hashtypes = Hashes::types_from_string(option.argument);
                if (hashtypes == 0) {
                    fprintf(stderr, "%s: illegal hash types '%s'\n", getprogname(), option.argument.c_str());
                    exit(1);
                }
            }
            else if (option.name == "no-directory-cache") {
                cache_directory = false;
            }
            else if (option.name == "only-files") {
                file_patterns.push_back(option.argument);
            }
            else if (option.name == "output") {
                dbname = option.argument;
            }
            else if (option.name == "prog-description") {
                dat.description = option.argument;
            }
            else if (option.name == "prog-name") {
                dat.name = option.argument;
            }
            else if (option.name == "prog-version") {
                dat.version = option.argument;
            }
            else if (option.name == "runtest") {
                runtest = true;
            }
            else if (option.name == "skip-files") {
                skip_files.insert(option.argument);
            }
            else if (option.name == "use-temp-directory") {
                flags |= OUTPUT_FL_TEMP;
            }
        }
        
        arguments = args.arguments;
    }
    catch (Exception &ex) {
        commandline.usage(false, stderr);
        exit(1);
    }

    if (arguments.size() > 1 && !dat.name.empty()) {
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
        if (dbname.empty()) {
            // TODO: make this work on Windows
            dbname = "/dev/stdout";
        }
    }

    if (dbname.empty()) {
        dbname = configuration.rom_db;
    }
    
    if (flags & OUTPUT_FL_TEMP) {
	dbname_real = dbname;
	auto var = tmpnam(tmpnam_buffer);
	if (var == nullptr) {
	    myerror(ERRSTR, "tmpnam() failed");
	    exit(1);
	}
        dbname = var;
    }

    try {
        if ((out = OutputContext::create(fmt, dbname, flags)) == nullptr) {
            exit(1);
        }

        if (!detector_name.empty()) {
    #if defined(HAVE_LIBXML2)
            seterrinfo(detector_name);
            auto detector = Detector::parse(detector_name);
            if (detector != nullptr) {
                out->detector(detector.get());
            }
    #else
            myerror(ERRDEF, "mkmamedb was built without XML support, detectors not available");
    #endif
        }


        /* TODO: handle errors */
        if (arguments.empty()) {
            if (!process_stdin(exclude, &dat, out.get())) {
                ret = 1;
            }
        }
        else {
            // TODO: this isn't overridable by --only-files?
            file_patterns.push_back(DEFAULT_FILE_PATTERNS);

            for (auto name : arguments) {
                auto last = name.find_last_not_of('/');
                if (last == std::string::npos) {
                    name = "/";
                }
                else {
                    name.resize(last + 1);
                }

                if (!process_file(name, exclude, &dat, file_patterns, skip_files, out.get(), flags)) {
                    ret = 1;
                }
            }
        }

        if (ret == 0) {
            out->close();
        }

        if (!configuration.roms_zipped) {
            CkmameDB::close_all();
        }

        if (flags & OUTPUT_FL_TEMP) {
            if (!rename_or_move(dbname, dbname_real)) {
                myerror(ERRDEF, "could not copy temporary output '%s' to '%s'", dbname.c_str(), dbname_real.c_str());
                return 1;
            }
        }
    }
    catch (const std::exception &exception) {
        fprintf(stderr, "%s: unexpected error: %s\n", getprogname(), exception.what());
        exit(1);
    }
            
    return ret;
}


static bool process_file(const std::string &fname, const std::unordered_set<std::string> &exclude, const DatEntry *dat, const std::vector<std::string> &file_patterns, const std::unordered_set<std::string> &files_skip, OutputContext *out, int flags) {
    struct zip *za;

    try {
	auto mdb = std::make_unique<RomDB>(fname, DBH_READ);
	return mdb->export_db(exclude, dat, out);
    }
    catch (std::exception &e) {
	/* that's fine */
    }

    if (configuration.roms_zipped && (za = zip_open(fname.c_str(), 0, nullptr)) != nullptr) {
	const char *name;
        auto ok = true;

	for (uint64_t i = 0; i < static_cast<uint64_t>(zip_get_num_entries(za, 0)); i++) {
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
                
                if (!Parser::parse(ps, exclude, dat, out, parser_flags)) {
                    ok = false;
                }
            }
            catch (Exception &e) {
                myerror(ERRFILE, "can't parse: %s", e.what());
                ok = false;
		continue;
	    }
 	}
	zip_close(za);

	return ok;
    }
    else {
	std::error_code ec;
	if (std::filesystem::is_directory(fname, ec)) {
            if (cache_directory) {
                CkmameDB::register_directory(fname);
            }

            auto ctx = ParserDir(nullptr, exclude, dat, out, parser_flags, fname, hashtypes, flags & OUTPUT_FL_RUNTEST);
            return ctx.parse();
	}

	if (ec) {
	    myerror(ERRDEF, "cannot stat() file '%s': %s", fname.c_str(), ec.message().c_str());
	    return false;
	}

	if (!configuration.roms_zipped) {
	    myerror(ERRDEF, "argument '%s' is not a directory", fname.c_str());
	    return false;
	}

        try {
            auto ps = std::make_shared<ParserSourceFile>(fname);
            return Parser::parse(ps, exclude, dat, out, parser_flags);
        }
        catch (std::exception &exception) {
            fprintf(stderr, "%s: can't process %s: %s\n", getprogname(), fname.c_str(), exception.what());
	    return false;
        }
    }
}


static bool process_stdin(const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *out) {
    try {
        auto ps = std::make_shared<ParserSourceFile>("");

        return Parser::parse(ps, exclude, dat, out, parser_flags);
    }
    catch (std::exception &exception) {
        fprintf(stderr, "%s: can't process stdin: %s\n", getprogname(), exception.what());
        return false;
    }
}
