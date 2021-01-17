#ifndef HAD_PARSE_H
#define HAD_PARSE_H

/*
  parse.h -- parser interface
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


#include <cstdio>
#include <unordered_set>

#include "dat.h"
#include "game.h"
#include "output.h"
#include "parser_source.h"
#include "romdb.h"

enum parser_state { PARSE_IN_HEADER, PARSE_IN_GAME, PARSE_IN_FILE, PARSE_OUTSIDE };

typedef enum parser_state parser_state_t;

#define PARSE_FL_ROM_DELETED 1
#define PARSE_FL_ROM_IGNORE 2
#define PARSE_FL_ROM_CONTINUED 4

#define PARSER_FL_FULL_ARCHIVE_NAME 1

class ParserContext {
public:
    static bool parse(ParserSourcePtr source, const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *output, int flags);

    /* TODO: move out of context */
    size_t lineno; /* current line number in input file */

    bool full_archive_name;

    // callbacks
    bool eof();
    bool file_continue(filetype_t ft);
    bool file_end(filetype_t ft);
    bool file_status(filetype_t ft, const std::string &attr);
    bool file_hash(filetype_t ft, int ht, const std::string &attr);
    bool file_ignore(filetype_t ft);
    bool file_merge(filetype_t ft, const std::string &attr);
    bool file_mtime(filetype_t ft, time_t mtime);
    bool file_name(filetype_t ft, const std::string &attr);
    bool file_size(filetype_t ft, const std::string &attr);
    bool file_size(filetype_t ft, uint64_t size);
    bool file_start(filetype_t ft);
    bool game_cloneof(filetype_t ft, const std::string &attr);
    bool game_description(const std::string &attr);
    bool game_end();
    bool game_name(const std::string &attr);
    bool game_start();
    bool prog_description(const std::string &attr);
    bool prog_header(const std::string attr);
    bool prog_name(const std::string &attr);
    bool prog_version(const std::string &attr);

    ParserContext(ParserSourcePtr source, const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *output_, int flags);
    ~ParserContext();
    
    bool parse_cm();
    bool parse_dir(const std::string &dname, int hashtypes);
    bool parse_rc();
    bool parse_xml();
    
private:

    bool header_end();
    void disk_end();
    void rom_end(filetype_t);
    bool ignore_game(const std::string &name);

    /* config */
    std::unordered_set<std::string> ignore;
    DatEntry dat_default;

    /* output */
    OutputContext *output;

    /* current source */
    ParserSourcePtr ps;

    /* state */
    int flags;
    parser_state_t state;
    DatEntry de; /* info about dat file */
    GamePtr g;      /* current game */
    File *r;      /* current ROM */
    Disk *d;      /* current disk */
};

#endif /* parse.h */
