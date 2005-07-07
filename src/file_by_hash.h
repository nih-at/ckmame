#ifndef HAD_FILE_BY_HASH_H
#define HAD_FILE_BY_HASH_H

/*
  $NiH$

  file_by_hash.h -- list of files with same hash
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



#include "hashes.h"
#include "types.h"



struct file_by_hash_entry {
    char *game;
    int index;
};

typedef struct file_by_hash_entry file_by_hash_entry_t;

struct file_by_hash {
    file_by_hash_entry_t *entry;
    int nentry;
};

typedef struct file_by_hash file_by_hash_t;

#define file_by_hash_length(f)	((f)->nentry)



void file_by_hash_free(file_by_hash_t *);
const char *file_by_hash_make_key(filetype_t, const hashes_t *);
int file_by_hash_default_hashtype(filetype_t);
file_by_hash_t *file_by_hash_new(void);

void file_by_hash_entry_free(file_by_hash_entry_t *);
file_by_hash_entry_t *file_by_hash_entry_new(const char *, int);

#endif /* file_by_hash.h */
