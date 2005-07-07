#ifndef HAD_HASHES_H
#define HAD_HASHES_H

/*
  $NiH: hashes.h,v 1.3 2005/07/04 23:51:32 dillo Exp $

  hashes.h -- hash related functions
  Copyright (C) 2004, 2005 Dieter Baron and Thomas Klausner

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



#include <sys/types.h>

#define HASHES_SIZE_CRC		4
#define HASHES_SIZE_MD5		16
#define HASHES_SIZE_SHA1	20
#define HASHES_SIZE_MAX		HASHES_SIZE_SHA1

#define HASHES_TYPE_CRC		1
#define HASHES_TYPE_MD5		2
#define HASHES_TYPE_SHA1	4
#define HASHES_TYPE_MAX		HASHES_TYPE_SHA1

#define HASHES_CMP_NOCOMMON	-1
#define HASHES_CMP_MATCH	0
#define HASHES_CMP_MISMATCH	1

struct hashes {
    int types;
    unsigned long crc;
    unsigned char md5[HASHES_SIZE_MD5];
    unsigned char sha1[HASHES_SIZE_SHA1];
};

typedef struct hashes hashes_t;

typedef struct hashes_update hashes_update_t;

#define hashes_has_type(h, t)	((h)->types & (t))



int hashes_cmp(const hashes_t *, const hashes_t *);
void hashes_init(hashes_t *);
void hashes_update(hashes_update_t *, const unsigned char *, size_t);
void hashes_update_final(hashes_update_t *);
struct hashes_update *hashes_update_new(hashes_t *);
int hash_from_string(hashes_t *, const char *);
const char *hash_to_string(char *, int, const hashes_t *);
const char *hash_type_string(int);

#endif /* hashes.h */
