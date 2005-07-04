/*
  $NiH: hashes.c,v 1.4 2005/06/22 22:11:28 dillo Exp $

  hashes.c -- utility functions for hash handling
  Copyright (C) 2004 Dieter Baron and Thomas Klausner

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

#include "types.h"
#include "romutil.h"

/*
  return 0 on match, 1 on mismatch, -1 on no common types
  XXX: use named constants
*/

int
hashes_cmp(const struct hashes *h1, const struct hashes *h2)
{
    if ((h1->types & h2->types) == 0)
	return -1;

    if (h1->types & h2->types & GOT_CRC)
	if (h1->crc != h2->crc)
	    return 1;

    if (h1->types & h2->types & GOT_MD5)
	if (memcmp(h1->md5, h2->md5, sizeof(h1->md5)) != 0)
	    return 1;

    if (h1->types & h2->types & GOT_SHA1)
	if (memcmp(h1->sha1, h2->sha1, sizeof(h1->sha1)) != 0)
	    return 1;

    return 0;
}



void
hashes_init(struct hashes *h)
{
    h->types = 0;
    h->crc = 0;
}
