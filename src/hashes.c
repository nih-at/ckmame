/*
  hashes.c -- utility functions for hash handling
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


#include <string.h>

#include "hashes.h"


hashes_cmp_t
hashes_cmp(const struct hashes *h1, const struct hashes *h2)
{
    if (h1->types == 0 || h2->types == 0)
	return HASHES_CMP_MATCH;

    if ((h1->types & h2->types) == 0)
	return HASHES_CMP_NOCOMMON;

    if (h1->types & h2->types & HASHES_TYPE_CRC)
	if (h1->crc != h2->crc)
	    return HASHES_CMP_MISMATCH;

    if (h1->types & h2->types & HASHES_TYPE_MD5)
	if (memcmp(h1->md5, h2->md5, sizeof(h1->md5)) != 0)
	    return HASHES_CMP_MISMATCH;

    if (h1->types & h2->types & HASHES_TYPE_SHA1)
	if (memcmp(h1->sha1, h2->sha1, sizeof(h1->sha1)) != 0)
	    return HASHES_CMP_MISMATCH;

    return HASHES_CMP_MATCH;
}


void
hashes_init(struct hashes *h)
{
    h->types = 0;
    h->crc = 0;
}
