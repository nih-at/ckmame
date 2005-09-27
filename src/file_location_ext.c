/*
  $NiH: file_location_ext.c,v 1.1.2.1 2005/08/06 17:48:46 wiz Exp $

  file_location_ext.c -- create / free file_location_ext structure
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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

#include "file_location.h"
#include "xmalloc.h"



void
file_location_ext_free(file_location_ext_t *e)
{
    if (e == NULL)
	return;

    free(e->name);
    free(e);
}



file_location_ext_t *
file_location_ext_new(const char *name, int idx, where_t where)
{
    file_location_ext_t *e;

    e = xmalloc(sizeof(*e));

    e->name = xstrdup(name);
    e->index = idx;
    e->where = where;

    return e;
}
