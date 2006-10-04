#ifndef _HAD_PMAP_H
#define _HAD_PMAP_H

/*
  $NiH: pmap.h,v 1.1 2006/04/16 00:12:57 dillo Exp $

  pmap.h -- hash table mapping strings to pointers
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



#include "dbl.h"

typedef void (*pmap_free_f)(void *);
typedef int (*pmap_foreach_f)(const char *, void *, void *);

struct pmap {
    DB *db;
    pmap_free_f cb_free;
};

typedef struct pmap pmap_t;



int pmap_add(pmap_t *, const char *, void *);
int pmap_delete(pmap_t *, const char *);
int pmap_foreach(pmap_t *, pmap_foreach_f, void *);
void pmap_free(pmap_t *);
void *pmap_get(pmap_t *, const char *);
pmap_t *pmap_new(pmap_free_f);

#endif /* pmap.h */
