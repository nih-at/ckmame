/*
  util.c -- utility functions
  Copyright (C) 1999 Dieter Baron and Thomas Klaunser

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xmalloc.h"
#include "util.h"
#include "error.h"

#define MAXROMPATH 128

#if 0
#define DEFAULT_ROMDIR "/usr/local/share/games/xmame" /* XXX: autoconfed */
#else
#define DEFAULT_ROMDIR "."
#endif

static char *rompath[MAXROMPATH] = { NULL };
static int rompath_init = 0;

static void init_rompath(void);



char *
findzip(char *name, int sample)
{
    int i;
    char b[8192];
    struct stat st;

    if (rompath_init == 0)
	init_rompath();

    for (i=0; rompath[i]; i++) {
	sprintf(b, "%s/%s/%s.zip",
		rompath[i], (sample ? "samples" : "roms"), name);
	if (stat(b, &st) == 0)
	    return xstrdup(b);
    }
    
    return NULL;
}



static void
init_rompath(void)
{
    int i, after;
    char *s, *e;

    /* skipping components placed via command line options */
    for (i=0; rompath[i]; i++)
	;

    if ((e = getenv("ROMPATH"))) {
	s = xstrdup(e);

	after = 0;
	if (s[0] == ':')
	    rompath[i++] = DEFAULT_ROMDIR;
	else if (s[strlen(s)-1] == ':')
	    after = 1;
	
	for (e=strtok(s, ":"); e; e=strtok(NULL, ":"))
	    rompath[i++] = e;

	if (after)
	    rompath[i++] = DEFAULT_ROMDIR;
    }
    else
	rompath[i++] = DEFAULT_ROMDIR;

    rompath[i] = NULL;

    rompath_init = 1;
}



int
strpcasecmp(char **sp1, char **sp2)
{
    return strcasecmp(*sp1, *sp2);
}
