/*
  $NiH$

  file_by_hash.c -- file_by_hash struct functions
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
#include "xmalloc.h"

struct file_by_hash *
file_by_hash_new(enum filetype ft, const struct hashes *hash)
{
    struct file_by_hash *fbh;

    fbh = (struct file_by_hash *)xmalloc(sizeof(struct file_by_hash));
    fbh->filetype = ft;
    memcpy(&fbh->hash, hash, sizeof(*hash));
    fbh->nentry = 0;
    fbh->entry = NULL;

    return fbh;
}



void
file_by_hash_free(struct file_by_hash *fbh)
{
    int i;

    if (fbh == NULL)
	return;

    if (fbh->nentry > 0) {
	for (i=0; i<fbh->nentry; i++)
	    free(fbh->entry[i].game);
	free(fbh->entry);
    }

    free(fbh);
}
