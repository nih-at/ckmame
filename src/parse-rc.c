/*
  $NiH$

  parse-rc.c -- parse Romcenter format files
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
#include <string.h>

#include "error.h"
#include "parse.h"
#include "util.h"
#include "xmalloc.h"



#define RC_SEP 0xac

enum section {
    RC_UNKNOWN = -1,
    RC_CREDITS,
    RC_DAT,
    RC_EMULATOR,
    RC_GAMES
};

intstr_t sections[] = {
    { RC_CREDITS,  "[CREDITS]" },
    { RC_DAT,      "[DAT]" },
    { RC_EMULATOR, "[EMULATOR]" },
    { RC_GAMES,    "[GAMES]" },
    { RC_GAMES,    "[RESOURCES]" },
    { RC_UNKNOWN,   NULL }
};

struct {
    enum section section;
    char *name;
    int (*cb)(parser_context_t *, const char *);
} fields[] = {
    { RC_CREDITS,  "version", parse_prog_version },
    { RC_EMULATOR, "refname", parse_prog_name },
    { RC_EMULATOR, "version", parse_prog_description }
};

int nfields = sizeof(fields) / sizeof(fields[0]);

static char *gettok(char **);
static int rc_romline(parser_context_t *, char *);



int
parse_rc(FILE *fin, parser_context_t *ctx)
{
    char *line, *val;
    enum section sect;
    int i;

    ctx->lineno = 0;
    sect = RC_UNKNOWN;

    rc_romline(ctx, NULL);

    while ((line=getline(fin))) {
	ctx->lineno++;

	if (line[0] == '[') {
	    sect = str2int(line, sections);
	    continue;
	}

	if (sect == RC_GAMES) {
	    if (rc_romline(ctx, line) < 0) {
		myerror(ERRFILE, "%d: cannot parse ROM line, skipping",
			ctx->lineno);
	    }
	else {
	    if ((val=strchr(line, '=')) == NULL) {
		myerror(ERRFILE, "%d: no `=' found",
			ctx->lineno);
		continue;
	    }
	    *(val++) = '\0';
	    
	    for (i=0; i<nfields; i++) {
		
		if (fields[i].section == sect
		    && strcmp(line, fields[i].name) == 0) {
		    fields[i].cb(ctx, val);
		    break;
		}
	    }
	}
    }

    return 0;
}



static char *
gettok(char **linep)
{
    char *p, *q;

    p = *linep;

    if (p == NULL)
	return NULL;

    q = strchr(p, RC_SEP);

    if (q)
	*(q++) = '\0';
    *linep = q;

    if (p[0] == '\0')
	return NULL;

    return p;
}



static int
rc_romline(parser_context_t *ctx, char *line)
{
    static char *gamename = NULL;

    char *p;
    char *parent, *name, *desc;

    if (line == NULL) {
	if (gamename) {
	    parse_game_end(ctx, 0);
	    free(gamename);
	}
	gamename = NULL;
	return 0;
    }

    if (gettok(&line) != NULL)
	return -1;

    parent = gettok(&line);
    (void)gettok(&line);
    name = gettok(&line);
    desc = gettok(&line);

    if (name == NULL)
	return -1;

    if (gamename == NULL || strcmp(gamename, name) != 0) {
	rc_romline(ctx, NULL); /* flush old game (if any) */

	gamename = xstrdup(name);
	parse_game_start(ctx, 0);
	parse_game_name(ctx, 0, 0, name);
	if (desc)
	    parse_game_description(ctx, desc);
	if (parent && strcmp(parent, name) != 0)
	    parse_game_cloneof(ctx, 0, 0, parent);
    }

    p = gettok(&line);
    if (p == NULL)
	return -1;

    parse_file_start(ctx, TYPE_ROM);
    parse_file_name(ctx, TYPE_ROM, 0, p);
    p = gettok(&line);
    if (p)
	parse_file_hash(ctx, TYPE_ROM, HASHES_TYPE_CRC, p);
    p = gettok(&line);
    if (p)
	parse_file_size(ctx, TYPE_ROM, 0, p);
    (void)gettok(&line);
    p = gettok(&line);
    if (p)
	parse_file_merge(ctx, TYPE_ROM, 0, p);
    parse_file_end(ctx, TYPE_ROM);

    return 0;
}
