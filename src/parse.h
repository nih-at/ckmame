#ifndef HAD_PARSE_H
#define HAD_PARSE_H

/*
  $NiH: parse.h,v 1.13 2007/04/12 21:09:20 dillo Exp $

  parse.h -- parser interface
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

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

#include "dat.h"
#include "dbl.h"
#include "game.h"
#include "output.h"

enum parser_state {
    PARSE_IN_HEADER,
    PARSE_IN_GAME,
    PARSE_IN_FILE,
    PARSE_OUTSIDE
};

typedef enum parser_state parser_state_t;

struct parser_context {
    /* config */
    const parray_t *ignore;
    dat_entry_t dat_default;

    /* output */
    output_context_t *output;

    /* current file */
    char *fname;
    /* XXX: move out of context */
    int lineno;			/* current line number in input file */

    /* state */
    parser_state_t state;
    dat_entry_t de;		/* info about dat file */
    game_t *g;			/* current game */
    rom_t *r;			/* current rom */
    disk_t *d;			/* current disk */
};

typedef struct parser_context parser_context_t;

/* parser functions */

int parse(const char *, const parray_t *, const dat_entry_t *,
	  output_context_t *);
int export_db(DB *, const parray_t *, const dat_entry_t *, output_context_t *);

/* backend parser functions */

int parse_cm(FILE *, parser_context_t *);
int parse_dir(const char *, parser_context_t *);
int parse_rc(FILE *, parser_context_t *);
int parse_xml(FILE *, parser_context_t *);

/* callbacks */

int parse_file_end(parser_context_t *, filetype_t);
int parse_file_status(parser_context_t *, filetype_t, int, const char *);
int parse_file_hash(parser_context_t *, filetype_t, int, const char *);
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
int parse_prog_header(parser_context_t *, const char *);
int parse_prog_name(parser_context_t *, const char *);
int parse_prog_version(parser_context_t *, const char *);

/* util functions */

int name_matches(const game_t *, const parray_t *);

#endif /* parse.h */
