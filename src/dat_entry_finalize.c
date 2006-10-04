/*
  $NiH: dat_entry_finalize.c,v 1.2 2006/03/17 10:59:27 dillo Exp $

  dat_entry_free.c -- free dat entry
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

#include <stdlib.h>

#include "dat.h"



void
dat_entry_finalize(dat_entry_t *de)
{
    free(de->name);
    free(de->description);
    free(de->version);
    
    de->name = de->description = de->version = NULL;
}
