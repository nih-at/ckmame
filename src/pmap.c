/*
  $NiH: pmap.c,v 1.3 2006/05/17 23:25:32 dillo Exp $

  pmap.c -- in-memory hash table mapping strings to pointers
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

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

#include <string.h>
#include <stdlib.h>

#include "dbl.h"
#include "pmap.h"
#include "xmalloc.h"

#define store(v, p)	((v)->size = sizeof(void *), (v)->data = &(p))

struct foreach_ud {
    pmap_foreach_f f;
    void *ud;
};

static void *extract(const DBT *);
static void *lookup(pmap_t *, DBT *);
static int iter_foreach(const DBT *, const DBT *, void *);
static int iter_free(const DBT *, const DBT *, void *);



int
pmap_add(pmap_t *pm, const char *key, void *val)
{
    DBT k, v;
    void *old;

    dbl_init_string_key(&k, key);
    store(&v, val);

    if (pm->cb_free && (old=lookup(pm, &k)) != NULL)
	pm->cb_free(old);

    return dbl_insert(pm->db, &k, &v);
}



int
pmap_delete(pmap_t *pm, const char *key)
{
    DBT k;
    void *data;

    dbl_init_string_key(&k, key);

    if (pm->cb_free && (data=lookup(pm, &k)) != NULL)
	pm->cb_free(data);

    return dbl_delete(pm->db, &k);
}



int
pmap_foreach(pmap_t *pm, pmap_foreach_f f, void *ud)
{
    struct foreach_ud fud;

    fud.f = f;
    fud.ud = ud;

    return dbl_foreach(pm->db, iter_foreach, &fud);
}



void
pmap_free(pmap_t *pm)
{
    if (pm == NULL)
	return;

    if (pm->cb_free)
	dbl_foreach(pm->db, iter_free, pm->cb_free);

    dbl_close(pm->db);
    free(pm);
}



void *
pmap_get(pmap_t *pm, const char *key)
{
    DBT k;

    dbl_init_string_key(&k, key);

    return lookup(pm, &k);
}



pmap_t *
pmap_new(pmap_free_f f)
{
    DB *db;
    pmap_t *pm;

    if ((db=dbl_open(NULL, DBL_READ|DBL_WRITE)) == NULL)
	return NULL;

    pm = (pmap_t *)xmalloc(sizeof(*pm));

    pm->db = db;
    pm->cb_free = f;

    return pm;
}



static void *
extract(const DBT *v)
{
    void *p;

    if (v->size != sizeof(void *))
	return NULL;

    memcpy((void *)&p, v->data, sizeof(void *));

    return p;
}



static int
iter_foreach(const DBT *k, const DBT *v, void *ud)
{
    struct foreach_ud *fud;
    char *s;
    int ret;

    fud = (struct foreach_ud *)ud;

    s = xmalloc(k->size+1);
    memcpy(s, k->data, k->size);
    s[k->size] = '\0';

    ret = fud->f(s, extract(v), fud->ud);

    free(s);
    return ret;
}



/*ARGSUSED1*/
static int
iter_free(const DBT *k, const DBT *v, void *ud)
{
    pmap_free_f f;

    f = (pmap_free_f)ud;

    f(extract(v));
    return 0;
}



static void *
lookup(pmap_t *pm, DBT *key)
{
    DBT val;

    if (dbl_lookup(pm->db, key, &val) != 0)
	return NULL;

    return extract(&val);
}
