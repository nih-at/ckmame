/*
  $NiH$

  hash_from_string.c -- convert string to hashes_t
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

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include "hashes.h"



int
hash_from_string(hashes_t *h, const char *str)
{
    int l;

    l = strlen(str);
    
    if (strspn(str, "0123456789ABCDEFabcdef") != l)
	return -1;

    switch(strlen(str)) {
    case HASHES_SIZE_CRC * 2:
	h->types = HASHES_TYPE_CRC;
	h->crc = strtoul(str, NULL, 16);
	break;

    case HASHES_SIZE_MD5 * 2:
	h->types = HASHES_TYPE_MD5;
	hex2bin(h->md5, str, HASHES_SIZE_MD5);
	break;

    case HASHES_SIZE_SHA1 * 2:
	h->types = HASHES_TYPE_SHA1;
	hex2bin(h->sha1, str, HASHES_SIZE_SHA1);
	break;

    default:
	return -1;
    }

    return 0;
}
