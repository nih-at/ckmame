/*
  $NiH: r_prog.c,v 1.5 2003/02/23 15:08:19 dillo Exp $

  r_prog.c -- read prog struct from db
  Copyright (C) 1999, 2003 Dieter Baron and Thomas Klausner

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



/* read list of strings from db */
#include <stdlib.h>
#include <string.h>

#include "dbh.h"
#include "r.h"
#include "xmalloc.h"



int
r_prog(DB *db, char **namep, char **versionp)
{
    int n;
    DBT v;
    void *data;

    if (ddb_lookup(db, "/prog", &v) != 0)
	return -1;
    
    data = v.data;

    *name = r__string(&v);
    *version = r__string(&v);

    free(data);

    return n;
}
