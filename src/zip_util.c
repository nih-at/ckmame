/*
  $NiH: util2.c,v 1.11 2006/04/26 21:01:51 dillo Exp $

  util.c -- utility functions needed only by ckmame itself
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



#include <errno.h>
#include <stdlib.h>

#include <zip.h>

#include "error.h"
#include "funcs.h"
#include "xmalloc.h"



struct zip *
my_zip_open(const char *name, int flags)
{
    struct zip *z;
    char errbuf[80];
    int err;

    z = zip_open(name, flags, &err);
    if (z == NULL) {
	zip_error_to_str(errbuf, sizeof(errbuf), err, errno);
	myerror(ERRDEF, "error %s zip archive `%s': %s",
		(flags & ZIP_CREATE ? "creating" : "opening"), name,
		errbuf);
    }

    return z;
}



#define RENAME_STRING "%s_renamed_by_ckmame_%d"

int
my_zip_rename(struct zip *za, int idx, const char *name)
{
    int zerr, idx2, try;
    char *name2;

    if (zip_rename(za, idx, name) == 0)
	return 0;

    zip_error_get(za, &zerr, NULL);

    if (zerr != ZIP_ER_EXISTS)
	return -1;

    idx2 = zip_name_locate(za, name, 0);

    if (idx2 == -1)
	return -1;

    name2 = xmalloc(strlen(RENAME_STRING)+strlen(name));

    for (try=0; try<10; try++) {
	sprintf(name2, RENAME_STRING, name, try);
	if (zip_rename(za, idx2, name2) == 0) {
	    free(name2);
	    return zip_rename(za, idx, name);
	}

	zip_error_get(za, &zerr, NULL);

	if (zerr != ZIP_ER_EXISTS) {
	    free(name2);
	    return -1;
	}
    }

    free(name2);
    return -1;
}
