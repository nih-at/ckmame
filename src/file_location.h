#ifndef HAD_FILE_LOCATION_H
#define HAD_FILE_LOCATION_H

/*
  file_location.h -- location of a file
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


struct file_location {
    char *name;
    int index;
};

typedef struct file_location file_location_t;

struct file_location_ext {
    char *name;
    int index;
    where_t where;
};

typedef struct file_location_ext file_location_ext_t;


#define file_location_name(a) ((a)->name)
#define file_location_index(a) ((a)->index)

const char *file_location_make_key(filetype_t, const Hashes *);
int file_location_default_hashtype(filetype_t);

int file_location_cmp(const file_location_t *, const file_location_t *);
void file_location_free(file_location_t *);
void file_location_finalize(file_location_t *);
file_location_t *file_location_new(const char *, int);

#define file_location_ext_name(a) ((a)->name)
#define file_location_ext_index(a) ((a)->index)
#define file_location_ext_where(a) ((a)->where)

void file_location_ext_free(file_location_ext_t *);
file_location_ext_t *file_location_ext_new(const char *, int, where_t);

#endif /* file_location.h */
