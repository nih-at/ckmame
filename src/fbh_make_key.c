/*
  $NiH: fbh_make_key.c,v 1.2 2005/06/26 23:11:28 dillo Exp $

  fbh_make_key.c -- make dbkey for file_by_hash struct
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

#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "romutil.h"
#include "hashes.h"
#include "xmalloc.h"

static const char *filetype_string(enum filetype);



char *
file_by_hash_make_key(enum filetype filetype, const struct hashes *hash)
{
    char *key;
    const char *ft, *ht;
    char *h;

    ft = filetype_string(filetype);
    ht = hash_type_string(hash->types);
    h = hash_to_string(hash->types, hash);

    if (ft == NULL || ht == NULL || h == NULL) {
	free(h);
	return NULL;
    }

    key = xmalloc(strlen(ft)+strlen(ht)+strlen(h)+4);
    sprintf(key, "/%s/%s/%s", ft, ht, h);

    free(h);

    return key;
}



static const char *
filetype_string(enum filetype filetype)
{
    /* XXX: I hate these fucking switch statements! */

    switch (filetype) {
    case TYPE_ROM:
	return "rom";

    case TYPE_SAMPLE:
	return "sample";

    case TYPE_DISK:
	return "disk";

    default:
	return NULL;
    }
}
