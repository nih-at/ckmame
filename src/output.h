#ifndef HAD_OUTPUT_H
#define HAD_OUTPUT_H

/*
  output.h -- output game info
  Copyright (C) 2006-2013 Dieter Baron and Thomas Klausner

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


#include "dat.h"
#include "game.h"
#include "detector.h"


typedef struct output_context output_context_t;

struct output_context {
    int (*close)(output_context_t *);
    int (*output_detector)(output_context_t *, detector_t *);
    int (*output_game)(output_context_t *, game_t *);
    int (*output_header)(output_context_t *, dat_entry_t *);
};

enum output_format {
    OUTPUT_FMT_CM,
    OUTPUT_FMT_DATAFILE_XML,
    OUTPUT_FMT_DB,
    OUTPUT_FMT_MTREE
};

typedef enum output_format output_format_t;

#define OUTPUT_FL_EXTENDED 1

output_context_t *output_cm_new(const char *, int);
output_context_t *output_db_new(const char *, int);
output_context_t *output_datafile_xml_new(const char *, int);
output_context_t *output_mtree_new(const char *, int);

int output_close(output_context_t *);
int output_detector(output_context_t *, detector_t *);
int output_game(output_context_t *, game_t *);
int output_header(output_context_t *, dat_entry_t *);

output_context_t *output_new(output_format_t, const char *, int);

/* for output_foo.c use only */
void output_cond_print_string(FILE *, const char *, const char *, const char *);
void output_cond_print_hash(FILE *, const char *, int, hashes_t *, const char *);

#endif /* output.h */
