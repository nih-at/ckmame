/*
  $NiH: file_location.c,v 1.3 2006/05/04 07:52:45 dillo Exp $

  file_location.c -- create / free file_location structure
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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

#include <stdlib.h>

#include "file_location.h"
#include "xmalloc.h"



int
file_location_cmp(const file_location_t *a, const file_location_t *b)
{
    int ret;
    
    ret = strcmp(file_location_name(a), file_location_name(b));
    if (ret == 0)
	ret = file_location_index(a) - file_location_index(b);

    return ret;
}




void
file_location_free(file_location_t *e)
{
    if (e == NULL)
	return;

    file_location_finalize(e);
    free(e);
}



file_location_t *
file_location_new(const char *name, int idx)
{
    file_location_t *e;

    e = xmalloc(sizeof(*e));

    e->name = xstrdup(name);
    e->index = idx;

    return e;
}
