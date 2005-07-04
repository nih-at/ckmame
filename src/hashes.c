/*
  $NiH: hashes.c,v 1.2 2005/07/04 22:41:36 dillo Exp $

  hashes.c -- utility functions for hash handling
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

#include "hashes.h"



int
hashes_cmp(const struct hashes *h1, const struct hashes *h2)
{
    if ((h1->types & h2->types) == 0)
	return HASHES_CMP_NOCOMMON;

    if (h1->types & h2->types & HASHES_TYPE_CRC)
	if (h1->crc != h2->crc)
	    return HASHES_CMP_MISMATCH;

    if (h1->types & h2->types & HASHES_TYPE_MD5)
	if (memcmp(h1->md5, h2->md5, sizeof(h1->md5)) != 0)
	    return HASHES_CMP_MISMATCH;

    if (h1->types & h2->types & HASHES_TYPE_SHA1)
	if (memcmp(h1->sha1, h2->sha1, sizeof(h1->sha1)) != 0)
	    return HASHES_CMP_MISMATCH;

    return HASHES_CMP_MATCH;
}



void
hashes_init(struct hashes *h)
{
    h->types = 0;
    h->crc = 0;
}
