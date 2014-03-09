/*
  hash_from_string.c -- convert string to hashes_t
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "hashes.h"
#include "util.h"


int
hash_from_string(hashes_t *h, const char *str)
{
    size_t l;
    int type;

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
	str += 2;

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
