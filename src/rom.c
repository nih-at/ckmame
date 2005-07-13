/*
  $NiH$

  rom.c -- initialize / finalize rom structure
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



#include <stdlib.h>

#include "rom.h"



void
rom_init(rom_t *r)
{
    r->name = r->merge = NULL;
    hashes_init(&r->hashes);
    r->size = 0;
    r->flags = FLAGS_OK;
    /* XXX: state */
    r->where = ROM_INZIP;
    r->altnames = NULL;
}



void
rom_finalize(rom_t *r)
{
    free(r->name);
    free(r->merge);
    parray_free(r->altnames, free);
}
