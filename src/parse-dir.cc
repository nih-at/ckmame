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
#include "Dir.h"
#include "error.h"
#include "globals.h"
#include "parse.h"
#include "util.h"


static bool parse_archive(ParserContext *ctx, filetype_t filetype, Archive *archive, int hashtypes);

// TODO: Make member of parser class once we have subclasses for the different parsers.
static std::string current_game;

// TODO: make methods.
static void end_game(ParserContext *ctx);
static void start_game(ParserContext *ctx, const std::string &name);

bool ParserContext::parse_dir(const std::string &dname, int hashtypes) {
    bool have_loose_files = false;

    lineno = 0;

    try {
	Dir dir(dname, false);
	std::filesystem::path filepath;

	if (roms_unzipped) {
	    while ((filepath = dir.next()) != "") {
		if (std::filesystem::is_directory(filepath)) {
		    /* TODO: handle errors */
		    auto a = Archive::open(filepath, TYPE_ROM, FILE_NOWHERE, ARCHIVE_FL_NOCACHE);
		    if (a) {
                        start_game(this, a->name);
			parse_archive(this, TYPE_ROM, a.get(), hashtypes);
                        end_game(this);
		    }
		}
		else {
		    if (std::filesystem::is_regular_file(filepath)) {
			/* TODO: always include loose files, separate flag? */
			if (full_archive_name) {
			    have_loose_files = true;
			}
			else {
			    myerror(ERRDEF, "found file '%s' outside of game subdirectory", filepath.c_str());
			}
		    }
		}
	    }

	    if (have_loose_files) {
		auto a = Archive::open_toplevel(dname, TYPE_ROM, FILE_NOWHERE, 0);

		if (a) {
                    start_game(this, a->name);
		    parse_archive(this, TYPE_ROM, a.get(), hashtypes);
                    end_game(this);
		}
	    }
	}
	else {
	    while ((filepath = dir.next()) != "") {
                if (std::filesystem::is_directory(filepath)) {
                    auto dir_empty = true;
                    
                    {
                        auto images = Archive::open(filepath, TYPE_DISK, FILE_NOWHERE, ARCHIVE_FL_NOCACHE);
                    
                        if (images && !images->is_empty()) {
                            dir_empty = false;
                            start_game(this, images->name);
                            parse_archive(this, TYPE_DISK, images.get(), Hashes::TYPE_ALL);
                        }
                    }
                    {
                        roms_unzipped = true;
                        auto files = Archive::open(filepath, TYPE_ROM, FILE_NOWHERE, ARCHIVE_FL_NOCACHE);
                        roms_unzipped = false;

                        if (files && !files->is_empty()) {
                            dir_empty = false;
                            for (auto &file : files->files) {
                                if (std::filesystem::path(file.name).extension() != ".chd") {
                                    myerror(ERRDEF, "skipping unknown file '%s/%s'", filepath.c_str(), file.name.c_str());
                                }
                            }
                        }
                    }
                    
                    if (dir_empty) {
                        myerror(ERRDEF, "skipping empty directory '%s'", filepath.c_str());
                    }
                }
                else {
                    switch (name_type(filepath)) {
                        case NAME_ZIP: {
                            /* TODO: handle errors */
                            auto a = Archive::open(filepath, TYPE_ROM, FILE_NOWHERE, ARCHIVE_FL_NOCACHE);
                            if (a) {
                                start_game(this, a->name.substr(0, a->name.length() - 4));
                                parse_archive(this, TYPE_ROM, a.get(), hashtypes);
                                end_game(this);
                            }
                            break;
                        }
                            
                        case NAME_IMAGES:
                            myerror(ERRDEF, "skipping top level disk image '%s'", filepath.c_str());
                            break;
                            
                        case NAME_CKMAMEDB:
                            // ignore
                            break;
                            
                        case NAME_UNKNOWN:
                            myerror(ERRDEF, "skipping unknown file '%s'", filepath.c_str());
                            break;
                    }
                }
	    }
	}

        end_game(this);

        eof();
    }
    catch (...) {
	return false;
    }

    return true;
}


static bool
parse_archive(ParserContext *ctx, filetype_t filetype, Archive *a, int hashtypes) {
    std::string name;

    for (size_t i = 0; i < a->files.size(); i++) {
        auto &file = a->files[i];

        if (filetype == TYPE_ROM) {
            a->file_compute_hashes(i, hashtypes);
        }

        ctx->file_start(filetype);
        ctx->file_name(filetype, file.name);
        ctx->file_size(filetype, file.size);
        ctx->file_mtime(filetype, file.mtime);
        if (file.status != STATUS_OK) {
            ctx->file_status(filetype, file.status == STATUS_BADDUMP ? "baddump" : "nodump");
	}
        for (int ht = 1; ht <= Hashes::TYPE_MAX; ht <<= 1) {
            if ((hashtypes & ht) && file.hashes.has_type(ht)) {
                ctx->file_hash(filetype, ht, file.hashes.to_string(ht));
	    }
	}
        ctx->file_end(filetype);
    }

    return true;
}

static void start_game(ParserContext *ctx, const std::string &name) {
    if (current_game == name) {
        return;
    }
    
    if (!current_game.empty()) {
        ctx->game_end();
    }
    
    ctx->game_start();
    ctx->game_name(std::filesystem::path(name).filename());
    current_game = name;
}

static void end_game(ParserContext *ctx) {
    if (!current_game.empty()) {
        ctx->game_end();
    }
    current_game = "";
}
