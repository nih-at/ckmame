/*
  $NiH: map.c,v 1.2 2006/04/15 22:52:58 dillo Exp $

  map.c -- in-memory hash table
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

#include <string.h>

#include "map.h"

struct foreach_ud {
    void (*vfn)(void *);
    int (*ifn)(const hashes_t *, parray_t *, void *);
    void *ud;
};

static int extract_key(hashes_t *, const DBT *);
static int foreach(const DBT *, const DBT *, void *);
static parray_t *lookup(map_t *, int, const hashes_t *, int);
static int make_key(DBT *, int, const hashes_t *);



int
map_add(map_t *map, int hashtype, const hashes_t *hashes, void *d)
{
    parray_t *pa;

    if ((pa=lookup(map, hashtype, hashes, 1)) == NULL)
	return -1;

    parray_push(pa, d);

    return 0;
}



int
map_foreach(map_t *map, int (*fn)(const hashes_t *, parray_t *, void *),
		void *ud)
{
    struct foreach_ud fud;

    fud.vfn = NULL;
    fud.ifn = fn;
    fud.ud = ud;
    
    return dbl_foreach(map, foreach, &fud);
}



void
map_free(map_t *map, void (*fn)(void *))
{
    struct foreach_ud fud;

    fud.vfn = fn;
    fud.ifn = NULL;
    fud.ud = NULL;
    
    dbl_foreach(map, foreach, &fud);
}



parray_t *
map_get(map_t *map, int hashtype, const hashes_t *hashes)
{
    return lookup(map, hashtype, hashes, 0);
}



map_t *
map_new(void)
{
    map_t *map;
    
    if ((map=dbl_open(NULL, DBL_READ|DBL_WRITE)) == NULL)
	return NULL;

    return map;
}



static int
extract_key(hashes_t *hashes, const DBT *k)
{
    switch (k->size) {
    case sizeof(hashes->crc):
	hashes->types = HASHES_TYPE_CRC;
	hashes->crc = *(unsigned long *)k->data;
	break;

    case HASHES_SIZE_MD5:
	hashes->types = HASHES_TYPE_MD5;
	memcpy(hashes->md5, k->data, HASHES_SIZE_MD5);
	break;

    case HASHES_SIZE_SHA1:
	hashes->types = HASHES_TYPE_SHA1;
	memcpy(hashes->md5, k->data, HASHES_SIZE_SHA1);
	break;

    default:
	return -1;
    }

    return 0;
}



static int
foreach(const DBT *k, const DBT *v, void *ud)
{
    struct foreach_ud *fud;
    parray_t *pa;
    hashes_t hashes;

    fud = ud;

    if (v->size != sizeof(pa))
	return -1;
    pa = *(parray_t **)v->data;
	
    if (fud->vfn) {
	parray_free(pa, fud->vfn);
	return 0;
    }
    else {
	if (extract_key(&hashes, k) < 0)
	    return -1;
	
	return fud->ifn(&hashes, pa, fud->ud);
    }
}



static parray_t *
lookup(map_t *map, int hashtype, const hashes_t *hashes, int create)
{
    DBT k, v;
    parray_t *pa;

    if (make_key(&k, hashtype, hashes) < 0)
	return NULL;
    
    if (dbl_lookup(map, &k, &v) != 0) {
	if (create) {
	    pa = parray_new();
	    v.size = sizeof(pa);
	    v.data = &pa;
	    if (dbl_insert(map, &k, &v) != 0) {
		parray_free(pa, NULL);
		return NULL;
	    }
	    return pa;
	}
	else
	    return NULL;
    }
    else {
	if (v.size != sizeof(pa))
	    return NULL;

	return *(parray_t **)v.data;
    }
}



static int
make_key(DBT *k, int hashtype, const hashes_t *hashes)
{
    if (!hashes_has_type(hashes, hashtype))
	return -1;

    switch (hashtype) {
    case HASHES_TYPE_CRC:
	k->size = sizeof(hashes->crc);
	k->data = (void *)&hashes->crc;
	break;
    case HASHES_TYPE_MD5:
	k->size = HASHES_SIZE_MD5;
	k->data = (void *)hashes->md5;
	break;
    case HASHES_TYPE_SHA1:
	k->size = HASHES_SIZE_SHA1;
	k->data = (void *)hashes->sha1;
	break;
    }

    return 0;
}

