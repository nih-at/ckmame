#ifndef HAD_PARSER_SOURCE_H
#define HAD_PARSER_SOURCE_H

/*
  parser_source.h -- reading parser input data from various sources
  Copyright (C) 2008-2014 Dieter Baron and Thomas Klausner

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
#include <zip.h>

typedef struct parser_source parser_source_t;


typedef int (*parser_source_close)(void *);
typedef parser_source_t *(*parser_source_open)(void *, const char *);
typedef ssize_t (*parser_source_read)(void *, void *, size_t);

int ps_close(parser_source_t *);
char *ps_getline(parser_source_t *);
parser_source_t *ps_new(void *, parser_source_close, parser_source_open,
			parser_source_read);
parser_source_t *ps_new_file(const char *);
parser_source_t *ps_new_stdin(void);
parser_source_t *ps_new_zip(const char *, struct zip *, const char *);
parser_source_t *ps_open(parser_source_t *, const char *);
int ps_peek(parser_source_t *);
ssize_t ps_read(parser_source_t *, void *, size_t);

#endif /* parser_source.h */
