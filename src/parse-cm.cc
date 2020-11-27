/*
  parse-cm.c -- parse listinfo/CMpro format files
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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
#include <string.h>

#include "error.h"
#include "parse.h"
#include "util.h"

enum parse_state { st_top, st_game, st_prog };

static char *gettok(char **);


int
parse_cm(parser_source_t *ps, parser_context_t *ctx) {
    char *cmd, *p, *l;
    enum parse_state state;

    ctx->lineno = 0;
    state = st_top;

    while ((l = ps_getline(ps))) {
	ctx->lineno++;

	cmd = gettok(&l);
	if (cmd == NULL)
	    continue;

	switch (state) {
	case st_top:
	    /* game/resource for MAME/Raine, machine for MESS */
	    if (strcmp(cmd, "game") == 0 || strcmp(cmd, "machine") == 0 || strcmp(cmd, "resource") == 0) {
		parse_game_start(ctx, TYPE_ROM);
		state = st_game;
	    }
	    else if (strcmp(cmd, "emulator") == 0 || strcmp(cmd, "clrmamepro") == 0)
		state = st_prog;
	    break;

	case st_game:
	    if (strcmp(cmd, "name") == 0)
		parse_game_name(ctx, TYPE_ROM, 0, gettok(&l));
	    else if (strcmp(cmd, "description") == 0)
		parse_game_description(ctx, gettok(&l));
	    else if (strcmp(cmd, "romof") == 0)
		parse_game_cloneof(ctx, TYPE_ROM, 0, gettok(&l));
	    else if (strcmp(cmd, "rom") == 0) {
		gettok(&l);
		if (strcmp(gettok(&l), "name") != 0) {
		    /* TODO: error */
		    myerror(ERRFILE, "%d: expected token (name) not found", ctx->lineno);
		    break;
		}
		parse_file_start(ctx, TYPE_ROM);
		parse_file_name(ctx, TYPE_ROM, 0, gettok(&l));

		/* read remaining tokens and look for known tokens */
		while ((p = gettok(&l)) != NULL) {
		    if (strcmp(p, "baddump") == 0 || strcmp(p, "nodump") == 0) {
			if (parse_file_status_(ctx, TYPE_ROM, 0, p) < 0)
			    break;
		    }
		    else if (strcmp(p, "crc") == 0 || strcmp(p, "crc32") == 0) {
			if ((p = gettok(&l)) == NULL) {
			    /* TODO: error */
			    myerror(ERRFILE, "%d: token crc missing argument", ctx->lineno);
			    break;
			}
			if (parse_file_hash(ctx, TYPE_ROM, Hashes::TYPE_CRC, p) < 0)
			    break;
		    }
		    else if (strcmp(p, "flags") == 0) {
			if ((p = gettok(&l)) == NULL) {
			    /* TODO: error */
			    myerror(ERRFILE, "%d: token flags missing argument", ctx->lineno);
			    break;
			}
			if (parse_file_status_(ctx, TYPE_ROM, 0, p) < 0)
			    break;
		    }
		    else if (strcmp(p, "merge") == 0) {
			if ((p = gettok(&l)) == NULL) {
			    myerror(ERRFILE, "%d: token merge missing argument", ctx->lineno);
			    break;
			}
			if (parse_file_merge(ctx, TYPE_ROM, 0, p) < 0)
			    break;
		    }
		    else if (strcmp(p, "md5") == 0) {
			if ((p = gettok(&l)) == NULL) {
			    myerror(ERRFILE, "%d: token md5 missing argument", ctx->lineno);
			    break;
			}
			if (parse_file_hash(ctx, TYPE_ROM, Hashes::TYPE_MD5, p) < 0)
			    break;
		    }
		    else if (strcmp(p, "sha1") == 0) {
			if ((p = gettok(&l)) == NULL) {
			    myerror(ERRFILE, "%d: token sha1 missing argument", ctx->lineno);
			    break;
			}
			if (parse_file_hash(ctx, TYPE_ROM, Hashes::TYPE_SHA1, p) < 0)
			    break;
		    }
		    else if (strcmp(p, "size") == 0) {
			if ((p = gettok(&l)) == NULL) {
			    /* TODO: error */
			    myerror(ERRFILE, "%d: token size missing argument", ctx->lineno);
			    break;
			}
			if (parse_file_size_(ctx, TYPE_ROM, 0, p) < 0)
			    break;
		    }
		    /*
		      else
		      myerror(ERRFILE, "%d: ignoring token '%s'", ctx->lineno, p);
		    */
		}

		parse_file_end(ctx, TYPE_ROM);
	    }
	    else if (strcmp(cmd, "disk") == 0) {
		gettok(&l);
		if (strcmp(gettok(&l), "name") != 0) {
		    /* TODO: error */
		    myerror(ERRFILE, "%d: expected token (name) not found", ctx->lineno);
		    break;
		}

		parse_file_start(ctx, TYPE_DISK);
		parse_file_name(ctx, TYPE_DISK, 0, gettok(&l));

		/* read remaining tokens and look for known tokens */
		while ((p = gettok(&l)) != NULL) {
		    if (strcmp(p, "sha1") == 0) {
			if ((p = gettok(&l)) == NULL) {
			    /* TODO: error */
			    myerror(ERRFILE, "%d: token sha1 missing argument", ctx->lineno);
			    break;
			}
			if (parse_file_hash(ctx, TYPE_DISK, Hashes::TYPE_SHA1, p) < 0)
			    break;
		    }
		    else if (strcmp(p, "md5") == 0) {
			if ((p = gettok(&l)) == NULL) {
			    /* TODO: error */
			    myerror(ERRFILE, "%d: token md5 missing argument", ctx->lineno);
			    break;
			}
			if (parse_file_hash(ctx, TYPE_DISK, Hashes::TYPE_MD5, p) < 0)
			    break;
		    }
                    else if (strcmp(p, "merge") == 0) {
                        if ((p = gettok(&l)) == NULL) {
                            myerror(ERRFILE, "%d: token merge missing argument", ctx->lineno);
                            break;
                        }
                        if (parse_file_merge(ctx, TYPE_DISK, 0, p) < 0)
                            break;
                    }
		    else if (strcmp(p, "flags") == 0) {
			if ((p = gettok(&l)) == NULL) {
			    /* TODO: error */
			    myerror(ERRFILE, "%d: token flags missing argument", ctx->lineno);
			    break;
			}
			if (parse_file_status_(ctx, TYPE_DISK, 0, p) < 0)
			    break;
		    }
		    /*
		      else
		      myerror(ERRFILE, "%d: ignoring token '%s'", ctx->lineno, p);
		    */
		}
		parse_file_end(ctx, TYPE_DISK);
	    }
	    else if (strcmp(cmd, "archive") == 0) {
		/* TODO: archive names */
	    }
	    else if (strcmp(cmd, ")") == 0) {
		parse_game_end(ctx, TYPE_ROM);
		state = st_top;
	    }
	    break;

	case st_prog:
	    if (strcmp(cmd, "name") == 0)
		parse_prog_name(ctx, gettok(&l));
	    else if (strcmp(cmd, "description") == 0)
		parse_prog_description(ctx, gettok(&l));
	    else if (strcmp(cmd, "version") == 0)
		parse_prog_version(ctx, gettok(&l));
	    else if (strcmp(cmd, "header") == 0)
		parse_prog_header(ctx, gettok(&l), 0);
	    else if (strcmp(cmd, ")") == 0)
		state = st_top;
	    break;
	}
    }

    return 0;
}


static char *
gettok(char **p) {
    char *s, *e;

    s = *p;

    if (s == NULL)
	return NULL;

    s += strspn(s, " \t");

    switch (*s) {
    case '\0':
    case '\n':
    case '\r':
	*p = NULL;
	return NULL;

    case '\"':
	s++;
	e = s + strcspn(s, "\"");
	break;

    default:
	e = s + strcspn(s, " \t\n\r");
	break;
    }

    if (*e != '\0') {
	*e = '\0';
	e++;
    }
    *p = e;
    return s;
}
