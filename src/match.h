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

class Match {
public:
    Match() : quality(QU_MISSING), where(FILE_NOWHERE), index(-1), offset(-1) { }
    
    quality_t quality;
    where_t where;
    
    ArchivePtr archive;
    int64_t index;

    /* for where == old */
    std::string old_game;
    std::string old_file;

    int64_t offset; /* offset of correct part if QU_LONG */
    
    std::string game() const;
    bool source_is_old() const { return where == FILE_OLD; }
    std::string file() const;
};

typedef struct Match match_t;

#define match_archive(m) ((m)->archive.get())
#define match_copy(m1, m2) ((m1) = (m2))
#define match_index(m) ((m)->index)
#define match_offset(m) ((m)->offset)
#define match_old_game(m) ((m)->old_game)
#define match_old_file(m) ((m)->old_file)
#define match_quality(m) ((m)->quality)
#define match_source_is_old(m) ((m)->source_is_old())
#define match_where(m) ((m)->where)

const char *match_file(match_t *);
void match_finalize(match_t *);
const char *match_game(match_t *);
void match_init(match_t *);
/*void match_merge(match_array_t *, Archive **, int, int); */
/*int matchcmp(const match_t *, const match_t *); */

#endif /* match.h */
