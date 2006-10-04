/*
  $NiH: zip_util.c,v 1.2 2006/05/06 23:31:40 dillo Exp $

  util.c -- utility functions needed only by ckmame itself
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



int
my_zip_rename(struct zip *za, int idx, const char *name)
{
    int zerr, idx2;
    char *name2;

    if (zip_rename(za, idx, name) == 0)
	return 0;

    zip_error_get(za, &zerr, NULL);

    if (zerr != ZIP_ER_EXISTS)
	return -1;

    idx2 = zip_name_locate(za, name, 0);

    if (idx2 == -1)
	return -1;

    if ((name2=my_zip_unique_name(za, name)) == NULL)
	return -1;
    
    if (zip_rename(za, idx2, name2) == 0) {
	free(name2);
	return zip_rename(za, idx, name);
    }

    free(name2);
    return -1;
}



char *
my_zip_unique_name(struct zip *za, const char *name)
{
    char *ret, *p;
    char n[4];
    const char *ext;
    int i;

    ret = (char *)xmalloc(strlen(name)+5);

    ext = strrchr(name, '.');
    if (ext == NULL) {
	strcpy(ret, name);
	p = ret+strlen(ret);
    }
    else {
	strncpy(ret, name, ext-name);
	p = ret + (ext-name);
	strcpy(p+4, ext);
    }	
    *(p++) = '-';

    for (i=0; i<1000; i++) {
	sprintf(n, "%03d", i);
	strncpy(p, n, 3);

	if (zip_name_locate(za, ret, 0) == -1)
	    return ret;
    }

    free(ret);
    return NULL;
}
