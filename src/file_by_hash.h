#ifndef HAD_FILE_BY_HASH_H
#define HAD_FILE_BY_HASH_H

/*
  file_by_hash.h -- list of files with same hash
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


#include "hashes.h"
#include "types.h"


struct file_by_hash {
    char *game;
    int index;
};

typedef struct file_by_hash file_by_hash_t;


#define file_by_hash_get(a, i) ((file_by_hash_t *)array_get((a), (i)))
#define file_by_hash_game(a, i) (file_by_hash_get((a), (i))->game)
#define file_by_hash_index(a, i) (file_by_hash_get((a), (i))->index)

const char *file_by_hash_make_key(filetype_t, const hashes_t *);
int file_by_hash_default_hashtype(filetype_t);

int file_by_hash_entry_cmp(const file_by_hash_t *, const file_by_hash_t *);
void file_by_hash_free(file_by_hash_t *);
void file_by_hash_finalize(file_by_hash_t *);
file_by_hash_t *file_by_hash_new(const char *, int);

#endif /* file_by_hash.h */
