#ifndef _HAD_W_H
#define _HAD_W_H

/*
  $NiH: w.h,v 1.8 2004/02/26 02:26:13 wiz Exp $

  w.h -- data base write functions
  Copyright (C) 1999, 2004 Dieter Baron and Thomas Klausner

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



void w__grow(DBT *, int);
void w__ushort(DBT *, unsigned short);
void w__ulong(DBT *, unsigned long);
void w__mem(DBT *, const char *, int);
void w__string(DBT *, const char *);
void w__pstring(DBT *, const void *);
void w__array(DBT *, void (*)(DBT *, void *), const void *, size_t, size_t);

void w__rom(DBT *, void *);
void w__disk(DBT *, void *);

int w_version(DB *);

#endif /* w.h */
