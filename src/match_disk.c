/*
  $NiH: match_disk.c,v 1.3 2006/05/01 21:09:11 dillo Exp $

  match_disk.c -- information about matches of files to disks
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



#include <stdlib.h>

#include "game.h"
#include "match_disk.h"
#include "xmalloc.h"



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



void
match_disk_set_source(match_disk_t *md, const disk_t *d)
{
    free(match_disk_name(md));
    match_disk_name(md) = xstrdup(disk_name(d));
    hashes_copy(match_disk_hashes(md), disk_hashes(d));
}
