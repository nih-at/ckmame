/*
  $NiH$

  rom_add_altname.c -- add alternate name to rom
  Copyright (C) 2004, 2005 Dieter Baron and Thomas Klausner

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

#include "game.h"
#include "xmalloc.h"



void
rom_add_altname(rom_t *r, const char *name)
{
    if (rom_altnames(r) == NULL)
	rom_altnames(r) = parray_new();
    parray_push(rom_altnames(r), xstrdup(name));
    parray_sort_unique(rom_altnames(r), strcasecmp);
}
