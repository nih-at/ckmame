/*
  hashes_update.c -- compute hashes
  Copyright (C) 2004-2014 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "config.h"

#include <limits.h>
#include <stdlib.h>
#ifdef HAVE_MD5INIT
#include <md5.h>
#else
#include "md5_own.h"
#endif
#ifdef HAVE_SHA1INIT
#include <sha1.h>
#else
#include "sha1_own.h"
#endif
#include <zlib.h>

#include "hashes.h"
#include "xmalloc.h"


struct hashes_update {
    uint32_t crc;
    MD5_CTX md5;
    SHA1_CTX sha1;

    struct hashes *h;
};


void
hashes_update(struct hashes_update *hu, const unsigned char *buf, size_t len)
{
    size_t i = 0;

    while (i < len) {
        size_t n = len - i > INT_MAX ? UINT_MAX : len - i;

        if (hu->h->types & HASHES_TYPE_CRC)
            hu->crc = (uint32_t)crc32(hu->crc, (const Bytef *)buf, (unsigned int )n);
        if (hu->h->types & HASHES_TYPE_MD5)
            MD5Update(&hu->md5, buf, (unsigned int )n);
        if (hu->h->types & HASHES_TYPE_SHA1)
            SHA1Update(&hu->sha1, buf, (unsigned int )n);

        i += n;
    }
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
	hu->crc = (uint32_t)crc32(0, NULL, 0);
    if (h->types & HASHES_TYPE_MD5)
	MD5Init(&hu->md5);
    if (h->types & HASHES_TYPE_SHA1)
	SHA1Init(&hu->sha1);

    return hu;
}
