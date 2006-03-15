#ifndef HAD_PARSE_H
#define HAD_PARSE_H

/*
  $NiH: parse.h,v 1.3 2005/09/27 21:33:03 dillo Exp $

  parse.h -- parser interface
  Copyright (C) 1999-2005 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

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
#include "map.h"

struct parser_context {
    DB *db;

    /* current file */
    FILE *fin;			/* input file */
    int lineno;			/* current line number in input file */
    char *prog_name;		/* name of dat file */
    char *prog_version;		/* version of dat file */
    game_t *g;			/* current game */

    /* accumulated info */
    dat_t *dat;
    map_t *map_rom;
    map_t *map_disk;
    int romhashtypes;
    int diskhashtypes;

    parray_t *lost_children;
    array_t *lost_children_types;

    parray_t *list[TYPE_MAX];
    
    /* state */
};

typedef struct parser_context parser_context_t;

/* parser functions */

int parse(parser_context_t *, const char *, const char *, const char *);
int parse_bookkeeping(parser_context_t *);
int parse_cm(parser_context_t *);
int parse_xml(parser_context_t *);

void parser_context_finalize_perfile(parser_context_t *);
void parser_context_free(parser_context_t *);
void parser_context_init_perfile(parser_context_t *);
parser_context_t *parser_context_new(DB *);

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
int parse_prog_name(parser_context_t *, const char *);
int parse_prog_version(parser_context_t *, const char *);

#endif /* parse.h */
