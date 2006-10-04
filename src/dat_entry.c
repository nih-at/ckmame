/*
  $NiH: dat_entry.c,v 1.1 2006/03/17 16:46:01 dillo Exp $

  dat_entry.c -- dat entry util functions
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

#include "dat.h"
#include "xmalloc.h"



void
dat_entry_init(dat_entry_t *de)
{
    de->name = de->description = de->version = NULL;
}



#define de_copy_member(X)					\
	(t->X = (hi && hi->X ? xstrdup(hi->X) 			\
		 : lo && lo->X ? xstrdup(lo->X) : NULL))

void
dat_entry_merge(dat_entry_t *t, const dat_entry_t *hi, const dat_entry_t *lo)
{
    dat_entry_init(t);

    de_copy_member(name);
    de_copy_member(description);
    de_copy_member(version);
}
