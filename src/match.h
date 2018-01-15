#ifndef HAD_MATCH_H
#define HAD_MATCH_H

/*
  match.h -- matching files with ROMs
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "parray.h"
#include "types.h"

struct match {
    quality_t quality;
    where_t where;
    union {
	struct {
	    archive_t *archive;
	    int index;
	} archive;
	struct {
	    char *game;
	    char *file;
	} old;
    } source;
    off_t offset; /* offset of correct part if QU_LONG */
};

typedef struct match match_t;

typedef array_t match_array_t;

#define match_array_free(ma) (array_free(ma, match_finalize))
#define match_array_get(ma, i) ((match_t *)array_get((ma), (i)))
#define match_array_new(n) (array_new_length(sizeof(match_t), (n), match_init))
#define match_array_length array_length

#define match_archive(m) ((m)->source.archive.archive)
#define match_copy(m1, m2) (memcpy(m1, m2, sizeof(match_t)))
#define match_free free
#define match_index(m) ((m)->source.archive.index)
#define match_offset(m) ((m)->offset)
#define match_old_game(m) ((m)->source.old.game)
#define match_old_file(m) ((m)->source.old.file)
#define match_quality(m) ((m)->quality)
#define match_source_is_old(m) (match_where(m) == FILE_OLD)
#define match_where(m) ((m)->where)


const char *match_file(match_t *);
void match_finalize(match_t *);
const char *match_game(match_t *);
void match_init(match_t *);
/*void match_merge(match_array_t *, archive_t **, int, int); */
/*int matchcmp(const match_t *, const match_t *); */

#endif /* match.h */
