/*
  match.c -- information about ROM/file matches
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


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "hashes.h"
#include "match.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"


const char *
match_file(match_t *m) {
    if (match_source_is_old(m)) {
	return match_old_file(m);
    }
    else {
        return file_name(&match_archive(m)->files[match_index(m)]);
    }
}


void
match_finalize(match_t *m) {
    if (match_source_is_old(m)) {
	free(match_old_game(m));
	free(match_old_file(m));
    }
}


const char *
match_game(match_t *m) {
    if (match_source_is_old(m))
	return match_old_game(m);
    else
	return match_archive(m)->name.c_str();
}


void
match_init(match_t *m) {
    match_quality(m) = QU_MISSING;
    match_where(m) = FILE_NOWHERE;
    match_archive(m) = NULL;
    match_index(m) = -1;
    match_offset(m) = -1;
}
