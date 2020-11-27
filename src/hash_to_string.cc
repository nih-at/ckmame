/*
  hash_to_string.c -- return string representation of hash
  Copyright (C) 2005-2014 Dieter Baron and Thomas Klausner

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


#include <stdio.h>
#include <string.h>

#include "compat.h"
#include "hashes.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"


const intstr_t hash_type_names[] = {{Hashes::TYPE_CRC, "crc"}, {Hashes::TYPE_MD5, "md5"}, {Hashes::TYPE_SHA1, "sha1"}, {0, NULL}};


const char *
hash_to_string(char *str, int type, const Hashes *hashes) {
    if (!hashes_has_type(hashes, type))
	return NULL;

    switch (type) {
    case Hashes::TYPE_CRC:
	sprintf(str, "%.8" PRIx32, hashes->crc);
	break;

    case Hashes::TYPE_MD5:
	bin2hex(str, hashes->md5, HASHES_SIZE_MD5);
	break;

    case Hashes::TYPE_SHA1:
	bin2hex(str, hashes->sha1, HASHES_SIZE_SHA1);
	break;

    default:
	return NULL;
    }

    return str;
}


