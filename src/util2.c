/*
  $NiH: util.c,v 1.2 2005/07/13 17:42:20 dillo Exp $

  util.c -- utility functions needed only by ckmame itself
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



#include <sys/stat.h>
#include <errno.h>

#include "funcs.h"
#include "globals.h"
#include "hashes.h"
#include "util.h"
#include "xmalloc.h"

#define MAXROMPATH 128
#define DEFAULT_ROMDIR "."

char *needed_dir = "needed";	/* XXX: proper value */
char *rompath[MAXROMPATH] = { NULL };
static int rompath_init = 0;



void
ensure_extra_file_map(void)
{
    extra_file_map = map_new();

    /* XXX: fill in */
}



void
ensure_needed_map(void)
{
    needed_map = map_new();

    /* XXX: fill in */
}



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



char *
make_needed_name(const rom_t *r)
{
    struct stat st;
    int i;
    char *s, crc[HASHES_SIZE_CRC*2+1];

    /* <needed_dir>/<crc>-nnn.zip */

    hash_to_string(crc, HASHES_TYPE_CRC, rom_hashes(r));
    
    s = xmalloc(strlen(needed_dir) + 18);
		
    for (i=0; i<1000; i++) {
	sprintf(s, "%s/%s-%03d.zip", needed_dir, crc, i);

	if (stat(s, &st) == -1 && errno == ENOENT)
	    return s;
    }

    free(s);

    /* XXX: better error handling */
    return NULL;
}



void
print_extra_files(const parray_t *files)
{
    int i;

    if (parray_length(files) == 0)
	return;

    printf("Extra files found:\n");
    
    for (i=0; i<parray_length(files); i++)
	printf("%s\n", (char *)parray_get(files, i));
}
