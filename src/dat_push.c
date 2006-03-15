/*
  $NiH$

  dat_push.c -- add dat entry
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

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

#include <string.h>

#include "dat.h"



void
dat_push(dat_t *d, const char *name, const char *version)
{
    dat_entry_t *de;

    array_grow(d, NULL);
    de = dat_get(d, dat_length(d)-1);

    de->name = name ? strdup(name) : NULL;
    de->version = version ? strdup(version) : NULL;
}
