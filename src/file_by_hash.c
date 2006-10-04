/*
  $NiH: file_by_hash.c,v 1.4 2006/05/04 07:52:45 dillo Exp $

  file_by_hash.c -- create / free file_by_hash structure
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

#include "file_by_hash.h"
#include "xmalloc.h"



int
file_by_hash_entry_cmp(const file_by_hash_t *a, const file_by_hash_t *b)
{
    int ret;
    
    ret = strcmp(a->game, b->game);
    if (ret == 0)
	ret = a->index - b->index;

    return ret;
}




void
file_by_hash_free(file_by_hash_t *e)
{
    if (e == NULL)
	return;

    file_by_hash_finalize(e);
    free(e);
}



file_by_hash_t *
file_by_hash_new(const char *game, int idx)
{
    file_by_hash_t *e;

    e = xmalloc(sizeof(*e));

    e->game = xstrdup(game);
    e->index = idx;

    return e;
}
