/*
  $NiH: w_game.c,v 1.4 2005/09/27 21:33:03 dillo Exp $

  w_game.c -- write game struct to db
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



/* write struct game to db */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "dbh.h"
#include "game.h"
#include "util.h"
#include "w.h"
#include "xmalloc.h"

static void w__hashes(DBT *, const hashes_t *);
static void w__rom(DBT *, const rom_t *);
static void w__rs(DBT *, const struct rs *);



int
w_game(DB *db, const game_t *game)
{
    DBT v;
    int err, i;

    v.data = NULL;
    v.size = 0;

    w__string(&v, game->description);
    w__ushort(&v, game->dat_no);
    for (i=0; i<GAME_RS_MAX; i++)
	w__rs(&v, game->rs+i);
	
    w__array(&v, w__disk, game->disks);

    err = ddb_insert(db, game->name, &v);

    free(v.data);

    return err;
}



static void
w__rs(DBT *v, const struct rs *rs)
{
    w__string(v, rs->cloneof[0]);
    w__string(v, rs->cloneof[1]);
    w__parray(v, (void (*)())w__string, rs->clones);
    w__array(v, (void (*)())w__rom, rs->files);
}



void
w__disk(DBT *v, const void *vd)
{
    const disk_t *d;

    d = (const disk_t *)vd;

    w__string(v, d->name);
    w__string(v, d->merge);
    w__hashes(v, &d->hashes);
    w__ushort(v, d->status);
}



static void
w__rom(DBT *v, const rom_t *r)
{
    w__string(v, r->name);
    w__string(v, r->merge);
    w__parray(v, (void (*)())w__string, r->altnames);
    w__hashes(v, &r->hashes);
    w__ulong(v, r->size);
    w__ushort(v, r->status);
    w__ushort(v, r->where);
}



static void
w__hashes(DBT *v, const hashes_t *h)
{
    w__ushort(v, h->types);
    if (h->types & HASHES_TYPE_CRC)
	w__ulong(v, h->crc);
    if (h->types & HASHES_TYPE_MD5)
	w__mem(v, h->md5, sizeof(h->md5));
    if (h->types & HASHES_TYPE_SHA1)
	w__mem(v, h->sha1, sizeof(h->sha1));
}
