/*
  $NiH: w_game.c,v 1.20 2004/04/24 09:40:25 dillo Exp $

  w_game.c -- write game struct to db
  Copyright (C) 1999, 2003, 2004 Dieter Baron and Thomas Klausner

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



/* write struct game to db */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "dbh.h"
#include "util.h"
#include "xmalloc.h"
#include "w.h"

static void w__hashes(DBT *, const struct hashes *);



int
w_game(DB *db, const struct game *game)
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
    w__array(&v, w__disk, game->disk, sizeof(struct disk), game->ndisk);

    err = ddb_insert(db, game->name, &v);

    free(v.data);

    return err;
}



void
w__disk(DBT *v, const void *vd)
{
    const struct disk *d;

    d = (const struct disk *)vd;

    w__string(v, d->name);
    w__hashes(v, &d->hashes);
}



void
w__rom(DBT *v, const void *vr)
{
    const struct rom *r;

    r = (const struct rom *)vr;

    w__string(v, r->name);
    w__string(v, r->merge);
    w__array(v, w__pstring, r->altname, sizeof(char *), r->naltname);
    w__hashes(v, &r->hashes);
    w__ulong(v, r->size);
    w__ushort(v, r->flags);
    w__ushort(v, r->where);
}



static void
w__hashes(DBT *v, const struct hashes *h)
{
    w__ushort(v, h->types);
    if (h->types & GOT_CRC)
	w__ulong(v, h->crc);
    if (h->types & GOT_MD5)
	w__mem(v, h->md5, sizeof(h->md5));
    if (h->types & GOT_SHA1)
	w__mem(v, h->sha1, sizeof(h->sha1));
}
