#ifndef _HAD_W_H
#define _HAD_W_H

/*
  $NiH: w.h,v 1.4 2006/10/04 17:36:44 dillo Exp $

  w.h -- data base write functions
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

void w__array(DBT *, void (*)(DBT *, const void *), const array_t *);
void w__disk(DBT *, const void *);
void w__grow(DBT *, int);
void w__mem(DBT *, const void *, unsigned int);
void w__parray(DBT *, void (*)(DBT *, const void *), const parray_t *);
void w__pstring(DBT *, const void *);
void w__string(DBT *, const char *);
void w__uint64(DBT *, uint64_t);
void w__uint8(DBT *, uint8_t);
void w__ushort(DBT *, unsigned short);
void w__ulong(DBT *, unsigned long);
int w_version(DB *);

#define w__uint16	w_ushort
#define w__uint32	w_ulong

#endif /* w.h */
