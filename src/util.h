#ifndef _HAD_UTIL_H
#define _HAD_UTIL_H

/*
  util.h -- miscellaneous utility functions
  Copyright (C) 1999-2021 Dieter Baron and Thomas Klausner

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
#include <vector>

#include <stdarg.h>

#include "printf_like.h"

enum name_type { NAME_ZIP, NAME_IMAGES, NAME_IGNORE, NAME_UNKNOWN };

typedef enum name_type name_type_t;

extern std::string rom_dir;

std::vector<uint8_t> hex2bin(const std::string &hex);
std::string bin2hex(const std::vector<uint8_t> &bin);
name_type_t name_type(const std::string &name);
bool ensure_dir(const std::string &name, bool strip_filename);
const std::string get_directory(void);
bool is_ziplike(const std::string &fname);
void print_human_number(FILE *f, uint64_t value);
std::string string_format(const char *format, ...) PRINTF_LIKE(1, 2);
std::string string_format_v(const char *format, va_list ap);

#endif
