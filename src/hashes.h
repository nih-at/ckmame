#ifndef HAD_HASHES_H
#define HAD_HASHES_H

/*
  hashes.h -- hash related functions
  Copyright (C) 2004, 2005 Dieter Baron and Thomas Klausner

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

#include "intstr.h"

#define HASHES_SIZE_CRC		4
#define HASHES_SIZE_MD5		16
#define HASHES_SIZE_SHA1	20
#define HASHES_SIZE_MAX		HASHES_SIZE_SHA1

#define HASHES_TYPE_CRC		1
#define HASHES_TYPE_MD5		2
#define HASHES_TYPE_SHA1	4
#define HASHES_TYPE_MAX		HASHES_TYPE_SHA1
#define HASHES_TYPE_ALL		((HASHES_TYPE_MAX<<1)-1)

enum hashes_cmp {
    HASHES_CMP_NOCOMMON = -1,
    HASHES_CMP_MATCH,
    HASHES_CMP_MISMATCH
};

typedef enum hashes_cmp hashes_cmp_t;

struct hashes {
    int types;
    unsigned long crc;
    unsigned char md5[HASHES_SIZE_MD5];
    unsigned char sha1[HASHES_SIZE_SHA1];
};

typedef struct hashes hashes_t;

typedef struct hashes_update hashes_update_t;


extern const intstr_t hash_type_names[];



#define hashes_are_crc_complement(h1, h2) \
	((((h1)->crc ^ (h2)->crc) & 0xffffffff) == 0xffffffff)
#define hashes_crc(h)		((h)->crc)
#define hashes_copy(h1, h2)	(memcpy((h1), (h2), sizeof(hashes_t)))
#define hashes_has_type(h, t)	(hashes_types(h) & (t))
#define hashes_types(h)		((h)->types)

#define hash_type_from_str(s)	(str2int((s), hash_type_names))
#define hash_type_string(i)	(int2str((i), hash_type_names))

hashes_cmp_t hashes_cmp(const hashes_t *, const hashes_t *);
void hashes_init(hashes_t *);
void hashes_update(hashes_update_t *, const unsigned char *, size_t);
void hashes_update_final(hashes_update_t *);
struct hashes_update *hashes_update_new(hashes_t *);
void hashes_set(hashes_t *, int, const unsigned char *);
int hashes_verify(const hashes_t *, int, const unsigned char *);
int hash_from_string(hashes_t *, const char *);
const char *hash_to_string(char *, int, const hashes_t *);
int hash_types_from_str(const char *);

#endif /* hashes.h */
