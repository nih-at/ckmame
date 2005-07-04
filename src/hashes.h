#ifndef HAD_HASHES_H
#define HAD_HASHES_H

/*
  $NiH: hashes.h,v 1.2 2005/07/04 22:41:36 dillo Exp $

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



#define HASHES_SIZE_MD5		16
#define HASHES_SIZE_SHA1	20

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

struct hashes_update;



int hashes_cmp(const struct hashes *, const struct hashes *);
void hashes_init(struct hashes *);
void hashes_update(struct hashes_update *, const unsigned char *, size_t);
void hashes_update_final(struct hashes_update *);
struct hashes_update *hashes_update_new(struct hashes *);
char *hash_to_string(int, const struct hashes *);
const char *hash_type_string(int);

#endif /* hashes.h */
