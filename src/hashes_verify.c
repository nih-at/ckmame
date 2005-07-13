/*
  $NiH$

  hashes_verify.c -- compare hash to memory
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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
hashes_verify(const hashes_t *h, int type, const unsigned char *b)
{
    const unsigned char *t;
    int s;

    switch (type) {
    case HASHES_TYPE_CRC:
	t = (const unsigned char *)&h->crc;
	s = sizeof(h->crc);
	break;

    case HASHES_TYPE_MD5:
	t = h->md5;
	s = HASHES_SIZE_MD5;
	break;

    case HASHES_TYPE_SHA1:
	t = h->sha1;
	s = HASHES_SIZE_SHA1;
	break;

    default:
	return -1;
    }

    return memcmp(t, b, s) == 0;
}
