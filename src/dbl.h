#ifndef _HAD_DBL_H
#define _HAD_DBL_H

/*
  $NiH: dbl.h,v 1.7 2006/04/16 00:12:56 dillo Exp $

  dbl.h -- abstraction of data base access functions
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



#include DBL_INCLUDE

#define DBL_READ	0x0	/* open readonly */
#define DBL_WRITE	0x1	/* open for writing */
#define DBL_NEW		0x2	/* create new database */



int dbl_close(DB *);
int dbl_delete(DB *, const DBT *);
const char *dbl_error(void);
int dbl_foreach(DB *, int (*)(const DBT *, const DBT *, void *), void *);
void dbl_init_string_key(DBT *, const char *);
int dbl_insert(DB *, DBT *, const DBT *);
int dbl_lookup(DB *, DBT *, DBT *);
DB* dbl_open(const char *, int);

#endif /* dbl.h */
