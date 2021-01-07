/*
  parse-dir.c -- read info from zip archives
  Copyright (C) 2006-2014 Dieter Baron and Thomas Klausner

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


#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "dir_old.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "parse.h"
#include "util.h"
#include "xmalloc.h"


static bool parse_archive(ParserContext *, Archive *, int hashtypes);


bool ParserContext::parse_dir(const std::string &dname, int hashtypes) {
    dir_t *dir;
    char b[8192];
    dir_status_t ds;
    struct stat st;
    bool have_loose_files = false;

    lineno = 0;

    if ((dir = dir_open(dname.c_str(), roms_unzipped ? 0 : DIR_RECURSE)) == NULL) {
        return false;
    }

    if (roms_unzipped) {
	while ((ds = dir_next(dir, b, sizeof(b))) != DIR_EOD) {
	    if (ds == DIR_ERROR) {
		myerror(ERRSTR, "error reading directory entry '%s', skipped", b);
		continue;
	    }

	    if (stat(b, &st) < 0) {
		myerror(ERRSTR, "can't stat '%s', skipped", b);
		/* TODO: handle error */
		continue;
	    }

	    if (S_ISDIR(st.st_mode)) {
		/* TODO: handle errors */
                auto a = Archive::open(b, TYPE_ROM, FILE_NOWHERE, ARCHIVE_FL_NOCACHE);
                if (a) {
		    parse_archive(this, a.get(), hashtypes);
		}
	    }
	    else {
		if (S_ISREG(st.st_mode)) {
		    /* TODO: always include loose files, separate flag? */
		    if (full_archive_name) {
			have_loose_files = true;
		    }
		    else {
			myerror(ERRDEF, "found file '%s' outside of game subdirectory", b);
		    }
		}
	    }
	}

	if (have_loose_files) {
            auto a = Archive::open_toplevel(dname, TYPE_ROM, FILE_NOWHERE, 0);

	    if (a) {
		parse_archive(this, a.get(), hashtypes);
	    }
	}
    }
    else {
	while ((ds = dir_next(dir, b, sizeof(b))) != DIR_EOD) {
	    if (ds == DIR_ERROR) {
		myerror(ERRSTR, "error reading directory entry '%s', skipped", b);
		continue;
	    }
	    switch (name_type(b)) {
                case NAME_ZIP: {
                    /* TODO: handle errors */
                    auto a = Archive::open(b, TYPE_ROM, FILE_NOWHERE, ARCHIVE_FL_NOCACHE);
                    if (a) {
                        parse_archive(this, a.get(), hashtypes);
                    }
                    break;
                }

                case NAME_CHD:
                    /* TODO: include disks in dat */
                case NAME_UNKNOWN:
                    if (stat(b, &st) < 0) {
                        myerror(ERRSTR, "can't stat '%s', skipped", b);
                        break;
                    }
                    if (S_ISREG(st.st_mode)) {
                        myerror(ERRDEF, "skipping unknown file '%s'", b);
                    }
                    break;
            }
        }
    }

    dir_close(dir);

    eof();

    return true;
}


static bool
parse_archive(ParserContext *ctx, Archive *a, int hashtypes) {
    std::string name;

    ctx->game_start();

    if (ctx->full_archive_name) {
        name = a->name;
    }
    else {
        name = mybasename(a->name.c_str());
    }
    if (name.length() > 4 && name.substr(name.length() - 4) == ".zip") {
        name.resize(name.length() - 4);
    }
    ctx->game_name(name);

    for (size_t i = 0; i < a->files.size(); i++) {
        auto &file = a->files[i];

	a->file_compute_hashes(i, hashtypes);

        ctx->file_start(TYPE_ROM);
        ctx->file_name(TYPE_ROM, file.name);
        ctx->file_size(TYPE_ROM, file.size);
        ctx->file_mtime(TYPE_ROM, file.mtime);
        if (file.status != STATUS_OK) {
            ctx->file_status(TYPE_ROM, file.status == STATUS_BADDUMP ? "baddump" : "nodump");
	}
        for (int ht = 1; ht <= Hashes::TYPE_MAX; ht <<= 1) {
            if ((hashtypes & ht) && file.hashes.has_type(ht)) {
                ctx->file_hash(TYPE_ROM, ht, file.hashes.to_string(ht));
	    }
	}
        ctx->file_end(TYPE_ROM);
    }

    ctx->game_end();

    return true;
}
