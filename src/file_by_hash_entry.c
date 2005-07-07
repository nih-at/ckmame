/*
  $NiH$

  file_by_hash_entry.c -- create / free file_by_hash_entry structure
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

#include "file_by_hash.h"
#include "xmalloc.h"



void
file_by_hash_entry_free(file_by_hash_entry_t *e)
{
    if (e == NULL)
	return;
    
    free(e->game);
    free(e);
}



file_by_hash_entry_t *
file_by_hash_entry_new(const char *game, int idx)
{
    file_by_hash_entry_t *e;

    e = xmalloc(sizeof(*e));

    e->game = xstrdup(game);
    e->index = idx;

    return e;
}
