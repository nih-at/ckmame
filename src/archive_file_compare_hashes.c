/*
  $NiH: archive.c,v 1.15 2006/05/16 07:52:51 wiz Exp $

  archive_file_compare_hashes.c -- compare hashes with file in archive
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "globals.h"



int
archive_file_compare_hashes(archive_t *a, int i, const hashes_t *h)
{
    hashes_t *rh;

    rh = rom_hashes(archive_file(a, i));

    if ((hashes_types(rh) & hashes_types(h)) != hashes_types(h))
	archive_file_compute_hashes(a, i, hashes_types(h)|romhashtypes);

    if (rom_status(archive_file(a, i)) != STATUS_OK)
	return HASHES_CMP_NOCOMMON;

    return hashes_cmp(rh, h);
}
