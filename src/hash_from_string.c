/*
  $NiH: hash_from_string.c,v 1.2.2.1 2005/07/31 20:10:47 wiz Exp $

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
#include <string.h>

#include "hashes.h"
#include "util.h"



int
hash_from_string(hashes_t *h, const char *str)
{
    int l;
    int type;

    l = strlen(str);
    
    if (l % 2 != 0 || strspn(str, "0123456789ABCDEFabcdef") != l)
	return -1;

    switch (l/2) {
    case HASHES_SIZE_CRC:
	type = HASHES_TYPE_CRC;
	h->crc = strtoul(str, NULL, 16);
	break;

    case HASHES_SIZE_MD5:
	type = HASHES_TYPE_MD5;
	hex2bin(h->md5, str, HASHES_SIZE_MD5);
	break;

    case HASHES_SIZE_SHA1:
	type = HASHES_TYPE_SHA1;
	hex2bin(h->sha1, str, HASHES_SIZE_SHA1);
	break;

    default:
	return -1;
    }

    h->types |= type;
    return type;
}
