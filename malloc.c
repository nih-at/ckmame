/*
  $NiH: malloc.c,v 1.10 2003/12/27 23:18:39 wiz Exp $

  malloc.c -- malloc routines with exit on failure
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

#include "error.h"
#include "xmalloc.h"



void *
xmalloc(size_t size)
{
    void *p;

    if ((p=malloc(size)) == NULL) {
	myerror(ERRDEF, "malloc failure");
	exit(1);
    }

    return p;
}



char *
xstrdup(const char *str)
{
    char *p;

    if (str == NULL)
	return NULL;
    
    if ((p=malloc(strlen(str)+1)) == NULL) {
	myerror(ERRDEF, "malloc failure");
	exit(1);
    }

    strcpy(p, str);
    
    return p;
}


void *
xrealloc(void *p, size_t size)
{
    if (p == NULL)
	return xmalloc(size);

    if ((p=realloc(p, size)) == NULL) {
	myerror(ERRDEF, "malloc failure");
	exit(1);
    }

    return p;
}
