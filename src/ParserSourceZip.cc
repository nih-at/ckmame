/*
  ParserSourceZip.cc -- reading parser input data from zip archive
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

#include "ParserSourceZip.h"

#include <cerrno>
#include <filesystem>

#include "ParserSourceFile.h"
#include "globals.h"

ParserSourceZip::ParserSourceZip(const std::string &archive_name_, struct zip *za_, const std::string &fname, bool relaxed) : archive_name(archive_name_), za(za_), zf(nullptr) {
    zip_flags_t flags = relaxed ? ZIP_FL_NOCASE | ZIP_FL_NODIR : 0;

    zip_stat_t st;
    if (zip_stat(za, fname.c_str(), flags, &st) < 0 || (zf = zip_fopen(za, fname.c_str(), flags)) == nullptr) {
        int zer, ser;

        zip_error_get(za, &zer, &ser);
        
        switch (zer) {
            case ZIP_ER_NOENT:
                errno = ENOENT;
                break;
            case ZIP_ER_MEMORY:
                errno = ENOMEM;
                break;
            default:
                errno = EIO;
                break;
        }
        
        throw std::exception();
    }

    mtime = st.mtime;
    output.set_error_archive(archive_name, fname);
}

ParserSourceZip::~ParserSourceZip() {
    close();
}

bool ParserSourceZip::close() {
    if (zf == nullptr) {
        return true;
    }
    
    auto ok = zip_fclose(zf) == 0;

    zf = nullptr;
    
    return ok;
}


ParserSourcePtr ParserSourceZip::open(const std::string &name) {
    try {
        return static_cast<ParserSourcePtr>(std::make_shared<ParserSourceZip>(archive_name, za, name, true));
    } catch (std::exception &e) {
        if (errno == ENOENT) {
            return static_cast<ParserSourcePtr>(std::make_shared<ParserSourceFile>(std::filesystem::path(archive_name).parent_path() / name));
        }
        throw e;
    }
}


size_t ParserSourceZip::read_xxx(void *data, size_t length) {
    if (zf == nullptr) {
        return 0;
    }
    
    auto done = zip_fread(zf, data, length);
    
    return done < 0 ? 0 : static_cast<size_t>(done);
}
