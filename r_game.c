/*
  r_game.c -- read game struct from db
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



/* read struct game from db */

#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "dbl.h"
#include "xmalloc.h"
#include "r.h"



struct game *
r_game(DB *db, char *name)
{
    DBT k, v;
    struct game *game;
    void *data;

    k.size = strlen(name);
    k.data = xmalloc(k.size);
    strncpy(k.data, name, k.size);

    if (ddb_lookup(db, &k, &v) != 0) {
	free(k.data);
	return NULL;
    }
    data = v.data;

    game = (struct game *)xmalloc(sizeof(struct game));
    
    game->name = xstrdup(name);
    game->description = r__string(&v);
    game->cloneof[0] = r__string(&v);
    game->cloneof[1] = r__string(&v);
    game->nclone = r__array(&v, r__pstring, (void *)&game->clone,
			    sizeof(char *));
    game->nrom = r__array(&v, r__rom, (void *)&game->rom, sizeof(struct rom));
    game->sampleof[0] = r__string(&v);
    game->sampleof[1] = r__string(&v);
    game->nsclone = r__array(&v, r__pstring, (void *)&game->sclone,
			    sizeof(char *));
    game->nsample = r__array(&v, r__rom, (void *)&game->sample,
			     sizeof(struct rom));

    free(k.data);
    free(data);

    return game;
}
