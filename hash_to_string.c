/*
  $NiH$

  hash_to_string.c -- return string representation of hash
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



#include <stdio.h>

#include "types.h"
#include "hashes.h"
#include "util.h"
#include "xmalloc.h"



char *
hash_to_string(int type, const struct hashes *hashes)
{
    char *str;

    if ((hashes->types & type) == 0)
	return NULL;

    switch (type) {
    case GOT_CRC:
	str = xmalloc(9);
	sprintf(str, "%.8lx", hashes->crc);
	return str;

    case GOT_MD5:
	return bin2hex(xmalloc(sizeof(hashes->md5)*2+1),
		       hashes->md5, sizeof(hashes->md5));

    case GOT_SHA1:
	return bin2hex(xmalloc(sizeof(hashes->sha1)*2+1),
		       hashes->sha1, sizeof(hashes->sha1));

    default:
	return NULL;
    }
}



const char *
hash_type_string(int type)
{
    switch (type) {
    case GOT_CRC:
	return "crc";

    case GOT_MD5:
	return "md5";

    case GOT_SHA1:
	return "sha1";

    default:
	return NULL;
    }
}
