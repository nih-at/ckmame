/*
  parser_source.c -- reading parser input data from various sources
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

#include "ParserSource.h"

#include <cstring>


#define PSBLKSIZE 1024

ParserSource::ParserSource() : current(nullptr), available(0) { }

ParserSource::~ParserSource() {
    close();
}


std::optional<std::string> ParserSource::getline() {
    for (;;) {
        char *p;
        if (available > 0 && (p = reinterpret_cast<char *>(memchr(current, '\n', available))) != nullptr) {
            auto line = reinterpret_cast<char *>(current);
            auto line_length = static_cast<size_t>(p - line);
            if (line_length > 0 && line[line_length - 1] == '\r') {
		line[line_length - 1] = '\0';
            }
            else {
		line[line_length] = '\0';
            }
            buffer_consume(line_length + 1);
            return line;
	}

        auto old_remaining = available;
        buffer_fill((available / PSBLKSIZE + 1) * PSBLKSIZE);

        if (old_remaining == available) {
            if (available == 0) {
                return {};
            }

            auto line = reinterpret_cast<char *>(current);
            line[available] = '\0';
            buffer_consume(available);
            return line;
	}
    }
}


int ParserSource::peek() {
    buffer_fill(1);

    if (available == 0) {
	return EOF;
    }

    return current[0];
}


size_t ParserSource::read(void *data, size_t length) {
    auto buffer = reinterpret_cast<uint8_t *>(data);

    size_t done = 0;
    
    if (available > 0) {
        done = std::min(available,  length);
	memcpy(buffer, current, done);
        buffer_consume(done);
        buffer += done;
	length -= done;
    }

    return done + read_xxx(buffer, length);
}


void ParserSource::buffer_consume(size_t length) {
    length = std::min(length, available);
    
    available -= length;
    current += length;

    if (available == 0) {
        current = data.data();
    }
}

// make N bytes available
void ParserSource::buffer_fill(size_t n) {
    if (available >= n) {
	return;
    }

    buffer_allocate(n);

    auto done = read_xxx(current + available, n - available);

    if (done > 0) {
        available += static_cast<size_t>(done);
    }
}

// make sure buffer is at least N + 1 bytes long (+1 for terminating NUL)
void ParserSource::buffer_allocate(size_t n) {
    if (available > 0 && current > data.data()) {
        memmove(data.data(), current, available);
        current = data.data();
    }

    auto new_size = n + 1;

    if (data.size() < new_size) {
        data.resize(new_size);
        current = data.data();
    }
}
