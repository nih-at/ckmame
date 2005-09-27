/*
  $NiH: parse-cm.c,v 1.2.2.1 2005/07/27 00:05:57 dillo Exp $

  parse-cm.c -- parse listinfo/CMpro format files
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
#include <string.h>

#include "error.h"
#include "parse.h"



enum parse_state { st_top, st_game, st_prog };

static char *gettok(char **);




int
parse_cm(parser_context_t *ctx)
{
    char b[8192], *cmd, *p, *l;
    enum parse_state state;
    
    ctx->lineno = 0;
    state = st_top;
    
    while (fgets(l=b, 8192, ctx->fin)) {
	ctx->lineno++;
	if (b[strlen(b)-1] != '\n') {
	    cmd = gettok(&l);
	    if ((cmd == NULL) || (strcmp(cmd, "history"))) {
		myerror(ERRFILE, "%d: warning: line too long (ignored)",
			ctx->lineno);
	    }
	    while (fgets(b, 8192, ctx->fin)) {
		if (b[strlen(b)-1] == '\n')
		    break;
	    }
	    continue;
	}
	
	cmd = gettok(&l);
	if (cmd == NULL)
	    continue;
	
	switch (state) {
	case st_top:
	    /* game/resource for MAME/Raine, machine for MESS */
	    if (strcmp(cmd, "game") == 0 || strcmp(cmd, "machine") == 0
		||strcmp(cmd, "resource") == 0) {
		parse_game_start(ctx, 0);
		state = st_game;
	    }
	    else if (strcmp(cmd, "emulator") == 0)
		state = st_prog;
	    break;
	    
	case st_game:
	    if (strcmp(cmd, "name") == 0)
		parse_game_name(ctx, 0, 0, gettok(&l));
	    else if (strcmp(cmd, "description") == 0)
		parse_game_description(ctx, gettok(&l));
	    else if (strcmp(cmd, "romof") == 0)
		parse_game_cloneof(ctx, TYPE_ROM, 0, gettok(&l));
	    else if (strcmp(cmd, "rom") == 0) {
		gettok(&l);
		if (strcmp(gettok(&l), "name") != 0) {
		    /* XXX: error */
		    myerror(ERRFILE, "%d: expected token (name) not found",
			    ctx->lineno);
		    break;
		}
		parse_file_start(ctx, TYPE_ROM);
		parse_file_name(ctx, TYPE_ROM, 0, gettok(&l));
		
		/* read remaining tokens and look for known tokens */
		while ((p=gettok(&l)) != NULL) {
		    if (strcmp(p, "crc") == 0 || strcmp(p, "crc32") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token crc missing argument",
				    ctx->lineno);
			    break;
			}
			if (parse_file_hash(ctx, TYPE_ROM,
					    HASHES_TYPE_CRC, p) < 0)
			    break;
		    }
		    else if (strcmp(p, "flags") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE,
				    "%d: token flags missing argument",
				    ctx->lineno);
			    break;
			}
			if (parse_file_status(ctx, TYPE_ROM, 0, p) < 0)
			    break;
		    }
		    else if (strcmp(p, "merge") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    myerror(ERRFILE,
				    "%d: token merge missing argument",
				    ctx->lineno);
			    break;
			}
			if (parse_file_merge(ctx, TYPE_ROM, 0, p) < 0)
			    break;
		    }
		    else if (strcmp(p, "md5") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    myerror(ERRFILE, "%d: token md5 missing argument",
				    ctx->lineno);
			    break;
			}
			if (parse_file_hash(ctx, TYPE_ROM,
					    HASHES_TYPE_MD5, p) < 0)
			    break;
		    }
		    else if (strcmp(p, "sha1") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    myerror(ERRFILE, "%d: token sha1 missing argument",
				    ctx->lineno);
			    break;
			}
			if (parse_file_hash(ctx, TYPE_ROM,
					    HASHES_TYPE_SHA1, p) < 0)
			    break;
		    }
		    else if (strcmp(p, "size") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token size missing argument",
				    ctx->lineno);
			    break;
			}
			if (parse_file_size(ctx, TYPE_ROM, 0, p) < 0)
			    break;
		    }
		    /*
		      else
		      myerror(ERRFILE, "%d: ignoring token `%s'", ctx->lineno, p);
		    */
		}

		parse_file_end(ctx, TYPE_ROM);
	    }
	    else if (strcmp(cmd, "disk") == 0) {
		gettok(&l);
		if (strcmp(gettok(&l), "name") != 0) {
		    /* XXX: error */
		    myerror(ERRFILE, "%d: expected token (name) not found",
			    ctx->lineno);
		    break;
		}
		
		parse_file_start(ctx, TYPE_DISK);
		parse_file_name(ctx, TYPE_DISK, 0, gettok(&l));

		/* read remaining tokens and look for known tokens */
		while ((p=gettok(&l)) != NULL) {
		    if (strcmp(p, "sha1") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token sha1 missing argument",
				    ctx->lineno);
			    break;
			}
			if (parse_file_hash(ctx, TYPE_DISK,
					    HASHES_TYPE_SHA1, p) < 0)
			    break;
		    }
		    else if (strcmp(p, "md5") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token md5 missing argument",
				    ctx->lineno);
			    break;
			}
			if (parse_file_hash(ctx, TYPE_DISK,
					    HASHES_TYPE_MD5, p) < 0)
			    break;
		    }
		    /*
		      else
		      myerror(ERRFILE, "%d: ignoring token `%s'", ctx->lineno, p);
		    */
		}
		parse_file_end(ctx, TYPE_DISK);
	    }
	    else if (strcmp(cmd, "sampleof") == 0)
		parse_game_cloneof(ctx, TYPE_SAMPLE, 0, gettok(&l));
	    else if (strcmp(cmd, "sample") == 0) {
		parse_file_start(ctx, TYPE_SAMPLE);
		parse_file_name(ctx, TYPE_SAMPLE, 0, gettok(&l));
		parse_file_end(ctx, TYPE_SAMPLE);
	    }
	    else if (strcmp(cmd, "archive") == 0) {
		/* XXX: archive names */
	    }
	    else if (strcmp(cmd, ")") == 0) {
		parse_game_end(ctx, 0);
		state = st_top;
	    }
	    break;
	    
	case st_prog:
	    if (strcmp(cmd, "name") == 0)
		parse_prog_name(ctx, gettok(&l));
	    else if (strcmp(cmd, "version") == 0)
		parse_prog_version(ctx, gettok(&l));
	    else if (strcmp(cmd, ")") == 0)
		state = st_top;
	    break;
	}
    }

    return 0;
}



static char *
gettok(char **p)
{
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
	e = s+strcspn(s, "\"");
	*e = 0;
	break;

    default:
	e = s+strcspn(s, " \t\n\r");
	*e = 0;
	break;
    }

    *p = e+1;
    return s;
}
