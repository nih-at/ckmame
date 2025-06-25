/*
  ParserSourceFile.cc -- reading parser input data from file
  Copyright (C) 2008-2019 Dieter Baron and Thomas Klausner

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

#include "ParserSourceFile.h"

#include <cstring>
#include <filesystem>
#include <sys/stat.h>

#include "Exception.h"
#include "globals.h"

ParserSourceFile::ParserSourceFile(const std::string& fname) : file_name(fname), f(nullptr) {
    if (!file_name.empty()) {
        f = make_shared_file(file_name, "r");
        if (!f) {
            throw Exception("can't open '%s': %s", fname.c_str(), strerror(errno));
        }
    }
    else {
        f = make_shared_stdin();
    }

    output.push_error_archive("", file_name);
}

ParserSourceFile::~ParserSourceFile() {
    output.pop_error_file_info();
    close();
}

bool ParserSourceFile::close() {
    auto ok = true;

    if (!file_name.empty() && f) {
        ok = fflush(f.get()) == 0;
    }

    f = nullptr;

    return ok;
}


ParserSourcePtr ParserSourceFile::open(const std::string& name) {
    std::string full_name;

    if (!file_name.empty()) {
        full_name = std::filesystem::path(file_name).parent_path() / name;
    }
    else {
        full_name = name;
    }

    return static_cast<ParserSourcePtr>(std::make_shared<ParserSourceFile>(full_name));
}


size_t ParserSourceFile::read_xxx(void* data, size_t length) {
    if (f == nullptr) {
        return 0;
    }

    return fread(data, 1, length, f.get());
}


time_t ParserSourceFile::get_mtime() {
    struct stat st;

    if (fstat(fileno(f.get()), &st) < 0) {
        return 0;
    }
    return st.st_mtime;
}


uint32_t ParserSourceFile::get_crc() {
    if (file_name.empty()) {
        return 0;
    }
    auto file = make_shared_file(file_name, "r");

    Hashes h;
    h.add_types(Hashes::TYPE_CRC);
    Hashes::Update hu(&h);

    size_t n;
    while (!feof(file.get())) {
        char buffer[8192];
        if ((n = fread(buffer, 1, sizeof(buffer), file.get())) < 0) {
            // TODO: handle error
            return 0;
        }
        if (n > 0) {
            hu.update(buffer, n);
        }
    }
    hu.end();
    return h.crc;
}
