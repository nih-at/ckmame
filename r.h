#ifndef _HAD_R_H
#define _HAD_R_H

/*
  r.h -- data base read functions
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



unsigned short r__ushort(DBT *v);
unsigned long r__ulong(DBT *v);
char *r__string(DBT *v);
void r__pstring(DBT *v, void *sp);
int r__array(DBT *v, void (*fn)(DBT *, void *), void **a, size_t size);

#include "types.h"

void r__rom(DBT *v, void *r);

#endif /* r.h */
