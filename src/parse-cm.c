/*
  $NiH: parse-cm.c,v 1.2 2005/06/12 19:30:11 dillo Exp $

  parse-cm.c -- parse listinfo/CMpro format files
  Copyright (C) 1999, 2001, 2002, 2003, 2004 Dieter Baron and Thomas Klausner

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
parse_cm(FILE *fin)
{
    char b[8192], *cmd, *p, *l;
    enum parse_state state;
    int lineno;
    
    lineno = 0;
    state = st_top;
    
    while (fgets(l=b, 8192, fin)) {
	lineno++;
	if (b[strlen(b)-1] != '\n') {
	    cmd = gettok(&l);
	    if ((cmd == NULL) || (strcmp(cmd, "history"))) {
		myerror(ERRFILE, "%d: warning: line too long (ignored)",
			lineno);
	    }
	    while (fgets(b, 8192, fin)) {
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
		parse_game_start();
		state = st_game;
	    }
	    else if (strcmp(cmd, "emulator") == 0)
		state = st_prog;
	    break;
	    
	case st_game:
	    if (strcmp(cmd, "name") == 0)
		parse_game_name(gettok(&l));
	    else if (strcmp(cmd, "description") == 0)
		parse_game_description(gettok(&l));
	    else if (strcmp(cmd, "romof") == 0)
		parse_game_cloneof(gettok(&l));
	    else if (strcmp(cmd, "rom") == 0) {
		gettok(&l);
		if (strcmp(gettok(&l), "name") != 0) {
		    /* XXX: error */
		    myerror(ERRFILE, "%d: expected token (name) not found",
			    lineno);
		    break;
		}
		parse_rom_start();
		parse_rom_name(gettok(&l));
		
		/* read remaining tokens and look for known tokens */
		while ((p=gettok(&l)) != NULL) {
		    if (strcmp(p, "crc") == 0 || strcmp(p, "crc32") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token crc missing argument",
				    lineno);
			    break;
			}
			if (parse_rom_crc(p) < 0) {
			    myerror(ERRFILE, "%d: %s", lineno, parse_errstr);
			    break;
			}
		    }
		    else if (strcmp(p, "flags") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE,
				    "%d: token flags missing argument",
				    lineno);
			    break;
			}
			if (parse_rom_flags(p) < 0) {
			    myerror(ERRFILE, "%d: %s", lineno, parse_errstr);
			    break;
			}
		    }
		    else if (strcmp(p, "merge") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    myerror(ERRFILE,
				    "%d: token merge missing argument",
				    lineno);
			    break;
			}
			if (parse_rom_merge(p) < 0) {
			    myerror(ERRFILE, "%d: %s", lineno);
			    break;
			}
		    }
		    else if (strcmp(p, "md5") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    myerror(ERRFILE, "%d: token md5 missing argument",
				    lineno);
			    break;
			}
			if (parse_rom_md5(p) < 0) {
			    myerror(ERRFILE, "%d: %s", lineno, parse_errstr);
			    break;
			}
		    }
		    else if (strcmp(p, "sha1") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    myerror(ERRFILE, "%d: token sha1 missing argument",
				    lineno);
			    break;
			}
			if (parse_rom_sha1(p) < 0) {
			    myerror(ERRFILE, "%d: %s", lineno, parse_errstr);
			    break;
			}
		    }
		    else if (strcmp(p, "size") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token size missing argument",
				    lineno);
			    break;
			}
			if (parse_rom_size(p) < 0) {
			    myerror(ERRFILE, "%d: %s", lineno, parse_errstr);
			    break;
			}
		    }
		    /*
		      else
		      myerror(ERRFILE, "%d: ignoring token `%s'", lineno, p);
		    */
		}

		parse_rom_end();
	    }
	    else if (strcmp(cmd, "disk") == 0) {
		gettok(&l);
		if (strcmp(gettok(&l), "name") != 0) {
		    /* XXX: error */
		    myerror(ERRFILE, "%d: expected token (name) not found",
			    lineno);
		    break;
		}
		
		parse_disk_start();
		parse_disk_name(gettok(&l));

		/* read remaining tokens and look for known tokens */
		while ((p=gettok(&l)) != NULL) {
		    if (strcmp(p, "sha1") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token sha1 missing argument",
				    lineno);
			    break;
			}
			if (parse_disk_sha1(p) < 0) {
			    myerror(ERRFILE, "%d: %s", lineno, parse_errstr);
			    break;
			}
		    }
		    else if (strcmp(p, "md5") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token md5 missing argument",
				    lineno);
			    break;
			}
			if (parse_disk_md5(p) < 0) {
			    myerror(ERRFILE, "%d: %s", lineno, parse_errstr);
			    break;
			}
		    }
		    /*
		      else
		      myerror(ERRFILE, "%d: ignoring token `%s'", lineno, p);
		    */
		}
		parse_disk_end();
	    }
	    else if (strcmp(cmd, "sampleof") == 0)
		parse_game_sampleof(gettok(&l));
	    else if (strcmp(cmd, "sample") == 0) {
		parse_sample_start();
		parse_sample_name(gettok(&l));
		parse_sample_end();
	    }
	    else if (strcmp(cmd, "archive") == 0) {
		/* XXX: archive names */
	    }
	    else if (strcmp(cmd, ")") == 0) {
		parse_game_end();
		state = st_top;
	    }
	    break;
	    
	case st_prog:
	    if (strcmp(cmd, "name") == 0)
		parse_prog_name(gettok(&l));
	    else if (strcmp(cmd, "version") == 0)
		parse_prog_version(gettok(&l));
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
