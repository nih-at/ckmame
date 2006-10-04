/*
  $NiH: r_game.c,v 1.6 2006/04/15 22:52:58 dillo Exp $

  r_game.c -- read game struct from db
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



/* read struct game from db */

#include <string.h>
#include <stdlib.h>

#include "dbh.h"
#include "game.h"
#include "r.h"
#include "xmalloc.h"

static void r__hashes(DBT *, hashes_t *);
static void r__rs(DBT *, struct rs *);



game_t *
r_game(DB *db, const char *name)
{
    DBT v;
    game_t *game;
    void *data;
    int i;

    if (dbh_lookup(db, name, &v) != 0)
	return NULL;

    data = v.data;

    game = xmalloc(sizeof(*game));
    
    game->name = xstrdup(name);
    game->description = r__string(&v);
    game->dat_no = r__ushort(&v);
    for (i=0; i<GAME_RS_MAX; i++)
	r__rs(&v, game->rs+i);
    game->disks = r__array(&v, r__disk, sizeof(disk_t));
    
    free(data);

    return game;
}



void
r__disk(DBT *v, void *vd)
{
    disk_t *d;
    
    d = vd;

    d->name = r__string(v);
    d->merge = r__string(v);
    r__hashes(v, &d->hashes);
    d->status = (status_t)r__ushort(v);
}



void
r__rom(DBT *v, void *vr)
{
    struct rom *r;
    
    r = (struct rom *)vr;

    r->name = r__string(v);
    r->merge = r__string(v);
    r->altnames = r__parray(v, (void *(*)())r__string);
    r__hashes(v, &r->hashes);
    r->size = r__ulong(v);
    r->status = (status_t)r__ushort(v);
    r->where = (where_t)r__ushort(v);
    /* XXX: r->state = ROM_0; */
}



static void
r__rs(DBT *v, struct rs *rs)
{
    rs->cloneof[0] = r__string(v);
    rs->cloneof[1] = r__string(v);
    rs->clones = r__parray(v, (void *(*)())r__string);
    rs->files = r__array(v, r__rom, sizeof(rom_t));
}



static void
r__hashes(DBT *v, struct hashes *h)
{
    h->types = r__ushort(v);
    if (h->types & HASHES_TYPE_CRC)
	h->crc = r__ulong(v);
    else
	h->crc = 0;
    if (h->types & HASHES_TYPE_MD5)
	r__mem(v, h->md5, sizeof(h->md5));
    if (h->types & HASHES_TYPE_SHA1)
	r__mem(v, h->sha1, sizeof(h->sha1));
}
