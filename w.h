#ifndef _HAD_W_H
#define _HAD_W_H

/*
  $NiH$

  w.h -- data base write functions
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



void w__grow(DBT *v, int n);
void w__ushort(DBT *v, unsigned short s);
void w__ulong(DBT *v, unsigned long l);
void w__string(DBT *v, char *s);
void w__pstring(DBT *v, void *sp);
void w__array(DBT *v, void (*fn)(DBT *, void *), void *a, size_t size, size_t n);

#include "types.h"
void w__rom(DBT *v, void *r);

#endif /* w.h */
