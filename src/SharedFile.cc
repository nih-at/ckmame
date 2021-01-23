/*
  SharedFile.cc -- class for automatic FILE * handling
  Copyright (C) 2021 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to shared_file rom sets for MAME.
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

#include "SharedFile.h"

static void file_deleter_close(FILE *f) {
    if (f != NULL) {
        fclose(f);
    }
}

static void file_deleter_noop(FILE *f) {
    return;
}

FILEPtr make_shared_file(const std::string &file_name, const std::string &flags) {
    auto fp = std::fopen(file_name.c_str(), flags.c_str());

    if (fp == NULL) {
	return NULL;
    }
    return std::shared_ptr<std::FILE>(fp, file_deleter_close);
}

FILEPtr make_shared_stdin() {
    return std::shared_ptr<std::FILE>(stdin, file_deleter_noop);
}

FILEPtr make_shared_stdout() {
    return std::shared_ptr<std::FILE>(stdout, file_deleter_noop);
}