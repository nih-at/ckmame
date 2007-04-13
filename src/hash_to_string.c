/*
  $NiH: hash_to_string.c,v 1.4 2006/10/04 17:36:44 dillo Exp $

  hash_to_string.c -- return string representation of hash
  Copyright (C) 2005-2007 Dieter Baron and Thomas Klausner

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



#include <stdio.h>
#include <string.h>

#include "types.h"
#include "hashes.h"
#include "util.h"
#include "xmalloc.h"



const intstr_t hash_type_names[] = {
    { HASHES_TYPE_CRC, "crc" },
    { HASHES_TYPE_MD5, "md5" },
    { HASHES_TYPE_SHA1, "sha1" },
    { 0, NULL }
};



const char *
hash_to_string(char *str, int type, const hashes_t *hashes)
{
    if (!hashes_has_type(hashes, type))
	return NULL;

    switch (type) {
    case HASHES_TYPE_CRC:
	sprintf(str, "%.8lx", hashes->crc);
	break;

    case HASHES_TYPE_MD5:
	bin2hex(str, hashes->md5, HASHES_SIZE_MD5);
	break;

    case HASHES_TYPE_SHA1:
	bin2hex(str, hashes->sha1, HASHES_SIZE_SHA1);
	break;

    default:
	return NULL;
    }

    return str;
}



int
hash_types_from_str(const char *s)
{
    const char *p, *q;
    char b[16];
    int types, t;

    types = 0;
    p = s;
    do {
	q = p+strcspn(p, ",");
	if ((size_t)(q-p) >= sizeof(b))
	    return 0;
	strlcpy(b, p, q-p+1);
	if ((t=hash_type_from_str(b)) == 0)
	    return 0;
	types |= t;

	p = q+1;
    } while (*q);
    
    return types;
}
