#ifndef HAD_PARSE_H
#define HAD_PARSE_H

/*
  parse.h -- parser interface
  Copyright (C) 1999-2013 Dieter Baron and Thomas Klausner

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

#include "dat.h"
#include "romdb.h"
#include "game.h"
#include "output.h"
#include "parser_source.h"

enum parser_state {
    PARSE_IN_HEADER,
    PARSE_IN_GAME,
    PARSE_IN_FILE,
    PARSE_OUTSIDE
};

typedef enum parser_state parser_state_t;

#define PARSE_FL_ROM_DELETED	1
#define PARSE_FL_ROM_IGNORE	2
#define PARSE_FL_ROM_CONTINUED	4

struct parser_context {
    /* config */
    const parray_t *ignore;
    dat_entry_t dat_default;

    /* output */
    output_context_t *output;

    /* current source */
    parser_source_t *ps;
    /* XXX: move out of context */
    int lineno;			/* current line number in input file */

    /* state */
    int flags;
    parser_state_t state;
    dat_entry_t de;		/* info about dat file */
    game_t *g;			/* current game */
    file_t *r;			/* current ROM */
    disk_t *d;			/* current disk */
};

typedef struct parser_context parser_context_t;

/* parser functions */

void parser_context_free(parser_context_t *);
parser_context_t *parser_context_new(parser_source_t *, const parray_t *,
				     const dat_entry_t *, output_context_t *);

int parse(parser_source_t *, const parray_t *, const dat_entry_t *, output_context_t *);
int export_db(romdb_t *, const parray_t *, const dat_entry_t *, output_context_t *);

/* backend parser functions */

int parse_cm(parser_source_t *, parser_context_t *);
int parse_dir(const char *, parser_context_t *, int);
int parse_rc(parser_source_t *, parser_context_t *);
int parse_xml(parser_source_t *, parser_context_t *);

/* callbacks */

int parse_file_continue(parser_context_t *, filetype_t, int, const char *);
int parse_file_end(parser_context_t *, filetype_t);
int parse_file_status(parser_context_t *, filetype_t, int, const char *);
int parse_file_hash(parser_context_t *, filetype_t, int, const char *);
int parse_file_ignore(parser_context_t *, filetype_t, int, const char *);
int parse_file_merge(parser_context_t *, filetype_t, int, const char *);
int parse_file_name(parser_context_t *, filetype_t, int, const char *);
int parse_file_size(parser_context_t *, filetype_t, int, const char *);
int parse_file_start(parser_context_t *, filetype_t);
int parse_game_cloneof(parser_context_t *, filetype_t, int, const char *);
int parse_game_description(parser_context_t *, const char *);
int parse_game_end(parser_context_t *, filetype_t);
int parse_game_name(parser_context_t *, filetype_t, int, const char *);
int parse_game_start(parser_context_t *, filetype_t);
int parse_prog_description(parser_context_t *, const char *);
int parse_prog_header(parser_context_t *, const char *, int);
int parse_prog_name(parser_context_t *, const char *);
int parse_prog_version(parser_context_t *, const char *);

/* util functions */

int name_matches(const char *, const parray_t *);

#endif /* parse.h */
