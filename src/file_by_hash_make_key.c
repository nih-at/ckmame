/*
  $NiH: file_by_hash_make_key.c,v 1.1 2005/07/07 22:00:20 dillo Exp $

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

#include "file_by_hash.h"
#include "xmalloc.h"

static int filetype_char(filetype_t);



int
file_by_hash_default_hashtype(filetype_t ft)
{
    if (ft == TYPE_DISK)	
	return HASHES_TYPE_MD5;
    else
	return HASHES_TYPE_CRC;
}



const char *
file_by_hash_make_key(filetype_t filetype, const hashes_t *hash)
{
    static char key[HASHES_SIZE_MAX*2 + 4];

    key[0] = '/';
    key[1] = filetype_char(filetype);
    key[2] = '/';
    hash_to_string(key+3, file_by_hash_default_hashtype(filetype), hash);

    return key;
}



static int
filetype_char(enum filetype filetype)
{
    /* XXX: I hate these fucking switch statements! */

    switch (filetype) {
    case TYPE_ROM:
	return 'r';

    case TYPE_SAMPLE:
	return 's';

    case TYPE_DISK:
	return 'd';

    default:
	return '?';
    }
}
