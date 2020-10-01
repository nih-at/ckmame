/*
  parse-rc.c -- parse Romcenter format files
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "parse.h"
#include "util.h"
#include "xmalloc.h"


#define RC_SEP 0xac

static int rc_plugin(parser_context_t *, const char *);


enum section { RC_UNKNOWN = -1, RC_CREDITS, RC_DAT, RC_EMULATOR, RC_GAMES };

intstr_t sections[] = {{RC_CREDITS, "[CREDITS]"}, {RC_DAT, "[DAT]"}, {RC_EMULATOR, "[EMULATOR]"}, {RC_GAMES, "[GAMES]"}, {RC_GAMES, "[RESOURCES]"}, {RC_UNKNOWN, NULL}};

struct {
    enum section section;
    const char *name;
    int (*cb)(parser_context_t *, const char *);
} fields[] = {{RC_CREDITS, "version", parse_prog_version}, {RC_DAT, "plugin", rc_plugin}, {RC_EMULATOR, "refname", parse_prog_name}, {RC_EMULATOR, "version", parse_prog_description}};

int nfields = sizeof(fields) / sizeof(fields[0]);

static char *gettok(char **);
static int rc_romline(parser_context_t *, char *);


int
parse_rc(parser_source_t *ps, parser_context_t *ctx) {
    char *line, *val;
    enum section sect;
    int i;

    ctx->lineno = 0;
    sect = RC_UNKNOWN;

    rc_romline(ctx, NULL);

    while ((line = ps_getline(ps))) {
	ctx->lineno++;

	if (line[0] == '[') {
	    sect = static_cast<enum section>(str2int(line, sections));
	    continue;
	}

	if (sect == RC_GAMES) {
	    if (rc_romline(ctx, line) < 0) {
		myerror(ERRFILE, "%d: cannot parse ROM line, skipping", ctx->lineno);
	    }
	}
	else {
	    if ((val = strchr(line, '=')) == NULL) {
		myerror(ERRFILE, "%d: no `=' found", ctx->lineno);
		continue;
	    }
	    *(val++) = '\0';

	    for (i = 0; i < nfields; i++) {
		if (fields[i].section == sect && strcmp(line, fields[i].name) == 0) {
		    fields[i].cb(ctx, val);
		    break;
		}
	    }
	}
    }

    return 0;
}


static char *
gettok(char **linep) {
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
rc_plugin(parser_context_t *ctx, const char *value) {
    myerror(ERRFILE, "%d: warning: RomCenter plugins not supported,", ctx->lineno);
    myerror(ERRFILE, "%d: warning: DAT won't work as expected.", ctx->lineno);
    return -1;
}


static int
rc_romline(parser_context_t *ctx, char *line) {
    static char *gamename = NULL;

    char *p;
    char *parent, *name, *desc;

    if (line == NULL) {
	if (gamename) {
	    parse_game_end(ctx, TYPE_ROM);
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
	parse_game_start(ctx, TYPE_ROM);
	parse_game_name(ctx, TYPE_ROM, 0, name);
	if (desc)
	    parse_game_description(ctx, desc);
	if (parent && strcmp(parent, name) != 0)
	    parse_game_cloneof(ctx, TYPE_ROM, 0, parent);
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
