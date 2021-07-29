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

#ifdef HAVE_MD5INIT
extern "C" {
#include <md5.h>
}
#else
#include "md5_own.h"
#endif
#ifdef HAVE_SHA1INIT
extern "C" {
#include <sha1.h>
}
#else
#include "sha1_own.h"
#endif

extern "C" {
#include <limits.h>
#include <stdlib.h>
#include <zlib.h>
}

#include "Hashes.h"

class HashesContexts {
public:
    uint32_t crc;
    MD5_CTX md5;
    SHA1_CTX sha1;
};

Hashes::Update::Update(Hashes *hashes_) : hashes(hashes_) {
    contexts = std::make_unique<HashesContexts>();

    if (hashes->has_type(TYPE_CRC)) {
        contexts->crc = static_cast<uint32_t>(crc32(0, NULL, 0));
    }
    if (hashes->has_type(TYPE_MD5)) {
        MD5Init(&contexts->md5);
    }
    if (hashes->has_type(TYPE_SHA1)) {
        SHA1Init(&contexts->sha1);
    }
}

Hashes::Update::~Update() {
    contexts = NULL;
}

void Hashes::Update::update(const void *data, size_t length) {
    size_t i = 0;

    while (i < length) {
	unsigned int n = length - i > UINT_MAX ? UINT_MAX : static_cast<unsigned int>(length - i);

        if (hashes->has_type(TYPE_CRC)) {
            contexts->crc = static_cast<uint32_t>(crc32(contexts->crc, static_cast<const Bytef *>(data), n));
        }
        if (hashes->has_type(TYPE_MD5)) {
            MD5Update(&contexts->md5, static_cast<const unsigned char *>(data), n);
        }
        if (hashes->has_type(TYPE_SHA1)) {
            SHA1Update(&contexts->sha1, static_cast<const uint8_t *>(data), n);
        }

        i += n;
    }
}


void Hashes::Update::end() {
    if (hashes->has_type(TYPE_CRC)) {
        hashes->crc = contexts->crc;
    }
    if (hashes->has_type(TYPE_MD5)) {
        MD5Final(hashes->md5.data(), &contexts->md5);
    }
    if (hashes->has_type(TYPE_SHA1)) {
        SHA1Final(hashes->sha1.data(), &contexts->sha1);
    }
}
