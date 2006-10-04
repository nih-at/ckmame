#ifndef _HAD_R_H
#define _HAD_R_H

/*
  $NiH: r.h,v 1.2 2005/07/13 17:42:20 dillo Exp $

  r.h -- data base read functions
  Copyright (C) 1999, 2004 Dieter Baron and Thomas Klausner

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

#include "array.h"
#include "parray.h"



array_t *r__array(DBT *, void (*)(DBT *, void *), size_t);
void r__disk(DBT *, void *);
void r__mem(DBT *, void *, unsigned int);
parray_t *r__parray(DBT *, void *(*)(DBT *));
void r__pstring(DBT *, void *);
void r__rom(DBT *, void *);
char *r__string(DBT *);
unsigned long r__ulong(DBT *);
unsigned short r__ushort(DBT *);


#endif /* r.h */
