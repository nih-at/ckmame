#ifndef HAD_PARSER_SOURCE_H
#define HAD_PARSER_SOURCE_H

/*
  parser_source.h -- reading parser input data from various sources
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

#include <cinttypes>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class ParserSource;

typedef std::shared_ptr<ParserSource> ParserSourcePtr;
    
class ParserSource {
public:
    ParserSource();
    virtual ~ParserSource();

    virtual bool close() { return true; }
    virtual size_t read_xxx(void *data, size_t length) = 0;
    virtual ParserSourcePtr open(const std::string &name) = 0;

    std::optional<std::string> getline();
    int peek();
    size_t read(void *data, size_t length);
    
private:
    std::vector<uint8_t> data;
    uint8_t *current; // current position in data buffer
    size_t available; // length of remaining valid data (from current)
    
    void buffer_consume(size_t n);
    void buffer_fill(size_t n);
    void buffer_allocate(size_t n); // ensure space for n bytes of valid data (from current)
};

#endif /* parser_source.h */
