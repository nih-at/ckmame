/*
  $NiH: w_game.c,v 1.11 2003/02/23 14:20:02 dillo Exp $

  w_game.c -- write game strcut to db
  Copyright (C) 1999, 2003 Dieter Baron and Thomas Klaunser

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



/* write struct game to db */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "dbh.h"
#include "util.h"
#include "xmalloc.h"
#include "w.h"



int
w_game(DB *db, struct game *game)
{
    int err;
    DBT v;

    v.data = NULL;
    v.size = 0;

    if (game->nclone) {
	qsort(game->clone, game->nclone, sizeof(char *),
	      (int (*)(const void *, const void *))strpcasecmp);
    }
    if (game->nsclone) {
	qsort(game->sclone, game->nsclone, sizeof(char *),
	      (int (*)(const void *, const void *))strpcasecmp);
    }
    
    w__string(&v, game->description);
    w__string(&v, game->cloneof[0]);
    w__string(&v, game->cloneof[1]);
    w__array(&v, w__pstring, game->clone, sizeof(char *), game->nclone);
    w__array(&v, w__rom, game->rom, sizeof(struct rom), game->nrom);
    w__string(&v, game->sampleof[0]);
    w__string(&v, game->sampleof[1]);
    w__array(&v, w__pstring, game->sclone, sizeof(char *), game->nsclone);
    w__array(&v, w__rom, game->sample, sizeof(struct rom), game->nsample);

    err = ddb_insert(db, game->name, &v);

    free(v.data);

    return err;
}



void
w__rom(DBT *v, void *vr)
{
    struct rom *r;

    r = (struct rom *)vr;

    w__string(v, r->name);
    w__string(v, r->merge);
    w__array(v, w__pstring, r->altname, sizeof(char *), r->naltname);
    w__ulong(v, r->size);
    w__ulong(v, r->crc);
    w__ushort(v, r->where);
}
