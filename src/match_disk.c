/*
  $NiH: match_disk.c,v 1.1.2.1 2005/07/27 00:05:57 dillo Exp $

  match_disk.c -- information about matches of files to disks
  Copyright (C) 1999, 2004, 2005 Dieter Baron and Thomas Klausner

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

#include "game.h"
#include "match_disk.h"



void
match_disk_finalize(match_disk_t *md)
{
    free(md->name);
}



void
match_disk_init(match_disk_t *md)
{
    match_disk_name(md) = NULL;
    hashes_init(match_disk_hashes(md));
    match_disk_quality(md) = QU_MISSING;
}
