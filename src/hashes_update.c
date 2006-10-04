/*
  $NiH: hashes_update.c,v 1.3 2006/05/12 22:12:18 dillo Exp $

  hashes_update.c -- compute hashes
  Copyright (C) 2004, 2005 Dieter Baron and Thomas Klausner

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



#include "config.h"

#include <stdlib.h>
#ifdef HAVE_MD5INIT
#include <md5.h>
#else
#include <md5_own.h>
#endif
#ifdef HAVE_SHA1INIT
#include <sha1.h>
#else
#include <sha1_own.h>
#endif
#include <zlib.h>

#include "hashes.h"
#include "xmalloc.h"



struct hashes_update {
    unsigned long crc;
    MD5_CTX md5;
    SHA1_CTX sha1;

    struct hashes *h;
};



void
hashes_update(struct hashes_update *hu, const unsigned char *buf, size_t len)
{
    if (hu->h->types & HASHES_TYPE_CRC)
	hu->crc = crc32(hu->crc, (const Bytef *)buf, len);
    if (hu->h->types & HASHES_TYPE_MD5)
	MD5Update(&hu->md5, buf, len);
    if (hu->h->types & HASHES_TYPE_SHA1)
	SHA1Update(&hu->sha1, buf, len);
}



void
hashes_update_final(struct hashes_update *hu)
{
    if (hu->h->types & HASHES_TYPE_CRC)
	hu->h->crc = hu->crc;
    if (hu->h->types & HASHES_TYPE_MD5)
	MD5Final(hu->h->md5, &hu->md5);
    if (hu->h->types & HASHES_TYPE_SHA1)
	SHA1Final(hu->h->sha1, &hu->sha1);

    free(hu);
}



struct hashes_update *
hashes_update_new(struct hashes *h)
{
    struct hashes_update *hu;

    hu = xmalloc(sizeof(*hu));

    hu->h = h;

    if (h->types & HASHES_TYPE_CRC)
	hu->crc = crc32(0, NULL, 0);
    if (h->types & HASHES_TYPE_MD5)
	MD5Init(&hu->md5);
    if (h->types & HASHES_TYPE_SHA1)
	SHA1Init(&hu->sha1);

    return hu;
}
