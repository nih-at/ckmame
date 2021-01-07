/*
  output.c -- output game info
  Copyright (C) 2006-2014 Dieter Baron and Thomas Klausner

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

#include "config.h"

#include "output.h"
#include "OutputContextCm.h"
#include "OutputContextDb.h"
#include "OutputContextMtree.h"
#include "OutputContextXml.h"
#include "error.h"


OutputContextPtr OutputContext::create(OutputContext::Format format, const std::string &fname, int flags) {
    switch (format) {
        case FORMAT_CM:
            return std::make_shared<OutputContextCm>(fname, flags);
            
        case FORMAT_DB:
            return std::make_shared<OutputContextDb>(fname, flags);
            
#if defined(HAVE_LIBXML2)
        case FORMAT_DATAFILE_XML:
            return std::make_shared<OutputContextXml>(fname, flags);
#endif
            
        case FORMAT_MTREE:
            return std::make_shared<OutputContextMtree>(fname, flags);
            
        default:
            return NULL;
    }
}


void OutputContext::cond_print_string(FILE *f, const char *pre, const std::string &str, const char *post) {
    const char *q;

    if (str.empty()) {
	return;
    }

    if (strcspn(str.c_str(), " \t") == str.length()) {
	q = "";
    }
    else {
	q = "\"";
    }

    fprintf(f, "%s%s%s%s%s", pre, q, str.c_str(), q, post);
}


void OutputContext::cond_print_hash(FILE *f, const char *pre, int t, const Hashes *h, const char *post) {
    cond_print_string(f, pre, h->to_string(t).c_str(), post);
}
