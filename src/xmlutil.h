#ifndef HAD_XMLUTIL_H
#define HAD_XMLUTIL_H

/*
  xmlutil.h -- parse XML file via callbacks
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

#include "myinttypes.h"
#include "parser_source.h"

typedef void (*xmlu_lineno_cb)(void *, int);
typedef int (*xmlu_attr_cb)(void *, int, int, const char *);
typedef int (*xmlu_tag_cb)(void *, int);
typedef int (*xmlu_text_cb)(void *, const char *);

struct xmlu_attr {
    const char *name;
    xmlu_attr_cb cb_attr;
    int arg1;
    int arg2;
};

typedef struct xmlu_attr xmlu_attr_t;

struct xmlu_entity {
    const char *name;
    const xmlu_attr_t *attr;
    xmlu_tag_cb cb_open;
    xmlu_tag_cb cb_close;
    xmlu_text_cb cb_text;
    int arg1;
};

typedef struct xmlu_entity xmlu_entity_t;


int xmlu_parse(parser_source_t *, void *, xmlu_lineno_cb, const xmlu_entity_t *, int);


#endif /* xmlutil.h */
