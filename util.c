/*
  $NiH: util.c,v 1.24 2004/04/20 23:42:09 dillo Exp $

  util.c -- utility functions
  Copyright (C) 1999, 2004 Dieter Baron and Thomas Klausner

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

char *rompath[MAXROMPATH] = { NULL };
static int rompath_init = 0;

void init_rompath(void);



char *
findfile(const char *name, enum filetype what)
{
    int i;
    char b[8192];
    struct stat st;

    if (rompath_init == 0)
	init_rompath();

    for (i=0; rompath[i]; i++) {
	sprintf(b, "%s/%s/%s%s",
		rompath[i],
		(what == TYPE_SAMPLE ? "samples" : "roms"),
		name,
		(what == TYPE_DISK ? ".chd" : ".zip"));
	if (stat(b, &st) == 0)
	    return xstrdup(b);
	if (what == TYPE_DISK) {
	    b[strlen(b)-4] = '\0';
	    if (stat(b, &st) == 0)
		return xstrdup(b);
	}
    }
    
    return NULL;
}



void
init_rompath(void)
{
    int i, after;
    char *s, *e;

    if (rompath_init)
	return;

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
strpcasecmp(const char * const *sp1, const char * const *sp2)
{
    return strcasecmp(*sp1, *sp2);
}



const char *
bin2hex(const char *s, int len)
{
    static char b[257];

    int i;

    if (len > sizeof(b)/2)
	len = sizeof(b)/2;

    for (i=0; i<len; i++)
	sprintf(b+2*i, "%02x", (unsigned char)s[i]);
    b[2*i] = '\0';

    return b;
}



#define HEX2BIN(c)	(((c)>='0' && (c)<='9') ? (c)-'0'	\
			 : ((c)>='A' && (c)<='F') ? (c)-'A'+10	\
			 : (c)-'a'+10)

int
hex2bin(char *t, const char *s, int tlen)
{
    int i;
    
    if (strspn(s, "0123456789AaBbCcDdEeFf") != tlen*2
	|| s[tlen*2] != '\0')
	return -1;

    for (i=0; i<tlen; i++)
	t[i] = HEX2BIN(s[i*2])<<4 | HEX2BIN(s[i*2+1]);
    
    return 0;
}
