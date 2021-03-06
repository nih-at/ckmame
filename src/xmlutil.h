#ifndef HAD_XMLUTIL_H
#define HAD_XMLUTIL_H

/*
  xmlutil.h -- parse XML file via callbacks
  Copyright (C) 1999-2020 Dieter Baron and Thomas Klausner

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

#include <string>
#include <unordered_map>

#include "ParserSource.h"

typedef void (*xmlu_lineno_cb)(void *, int);
typedef bool (*xmlu_attr_cb)(void *ctx, int file_type, int hash_type, const std::string &value);
typedef bool (*xmlu_tag_cb)(void *ctx, int file_type);
typedef bool (*xmlu_text_cb)(void *ctx, const std::string &value);

class XmluAttr {
 public:
    xmlu_attr_cb cb_attr;
    int arg1;
    int arg2;
};

class XmluEntity {
 public:
    XmluEntity(const std::unordered_map<std::string, XmluAttr> &attributes_, xmlu_tag_cb cb_open_ = NULL, xmlu_tag_cb cb_close_ = NULL, int arg1_ = 0) : attr(attributes_), cb_open(cb_open_), cb_close(cb_close_), cb_text(NULL), arg1(arg1_) { }
    XmluEntity(xmlu_text_cb cb_text_ = NULL, int arg1_ = 0) : cb_open(NULL), cb_close(NULL), cb_text(cb_text_), arg1(arg1_) { }
    std::unordered_map<std::string, XmluAttr> attr;
    xmlu_tag_cb cb_open;
    xmlu_tag_cb cb_close;
    xmlu_text_cb cb_text;
    int arg1;
};

int xmlu_parse(ParserSource *, void *ctx, xmlu_lineno_cb lineno_cb, const std::unordered_map<std::string, XmluEntity> &entities);

#endif /* xmlutil.h */
