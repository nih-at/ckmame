#ifndef HAD_FILE_H
#define HAD_FILE_H

/*
  file.h -- information about one file
  Copyright (C) 1999-2013 Dieter Baron and Thomas Klausner

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


#include <time.h>


#include "hashes.h"
#include "myinttypes.h"
#include "parray.h"
#include "types.h"

struct file_sh {
    uint64_t size;
    hashes_t hashes;
};

typedef struct file_sh file_sh_t;

enum {
    FILE_SH_FULL,
    FILE_SH_DETECTOR,
    FILE_SH_MAX
};
  
struct file {
    char *name;
    char *merge;
    file_sh_t sh[FILE_SH_MAX];
    time_t mtime;
    status_t status;
    where_t where;
};

typedef struct file file_t;


#define file_hashes(f)		(file_hashes_xxx((f), FILE_SH_FULL))
#define file_hashes_xxx(f, i)	(&(f)->sh[(i)].hashes)
#define file_merge(f)		((f)->merge)
#define file_mtime(f)           ((f)->mtime)
#define file_name(f)		((f)->name)
#define file_size(f)		(file_size_xxx((f), FILE_SH_FULL))
#define file_size_known(f)	(file_size_xxx_known((f), FILE_SH_FULL))
#define file_size_xxx(f, i)	((f)->sh[(i)].size)
#define file_size_xxx_known(f, i)	\
		(file_size_xxx((f), (i)) != SIZE_UNKNOWN)
#define file_status(f)		((f)->status)
#define file_where(f)		((f)->where)

#define file_compare_n(f1, f2)	(strcmp(file_name(f1), file_name(f2)) == 0)

bool file_compare_m(const file_t *, const file_t *);
bool file_compare_msc(const file_t *, const file_t *);
bool file_compare_nsc(const file_t *, const file_t *);
bool file_compare_sc(const file_t *, const file_t *);
void file_init(file_t *);
void file_finalize(file_t *);
bool file_sh_is_set(const file_t *, int);

#endif /* file.h */
