#ifndef HAD_PARSER_SOURCE_ZIP_H
#define HAD_PARSER_SOURCE_ZIP_H

/*
  ParserSourceZip.h -- reading parser input data from zip archive
  Copyright (C) 2008-2020 Dieter Baron and Thomas Klausner

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

#include "ParserSource.h"

#include <zip.h>

class ParserSourceZip : public ParserSource {
public:
    ParserSourceZip(const std::string &archive_name, struct zip *za, const std::string &file_name, bool relaxed = false);
    ~ParserSourceZip() override;
    
    bool close() override;
    ParserSourcePtr open(const std::string &name) override;
    size_t read_xxx(void *data, size_t length) override;
    time_t get_mtime() override {return mtime;}
    uint32_t get_crc() override {return crc;}

  private:
    std::string archive_name;
    struct zip *za;
    struct zip_file *zf;
    time_t mtime;
    uint32_t crc;
};

#endif // HAD_PARSER_SOURCE_ZIP_H
