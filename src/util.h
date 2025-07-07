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

#include <filesystem>
#include <string>
#include <vector>

#include <cstdarg>
#include <ctime>

#include "printf_like.h"

enum name_type { NAME_ZIP, NAME_IMAGES, NAME_IGNORE, NAME_UNKNOWN };

typedef enum name_type name_type_t;

std::vector<uint8_t> hex2bin(const std::string &hex);
std::string bin2hex(const std::vector<uint8_t> &bin);
bool string_less_case_insensitive(const std::string &lhs, const std::string &rhs);
std::string string_lower(const std::string &s);
bool string_starts_with(const std::string &large, const std::string &small);
name_type_t name_type(const std::filesystem::directory_entry &entry);
name_type_t name_type_s(const std::string &name);
void diff_lines(const std::vector<std::string>& old_lines, const std::vector<std::string>& new_lines, size_t& added, size_t& removed);
bool ensure_dir(const std::filesystem::path& name, bool strip_filename); // TODO: replace with ensure_directory
void ensure_directory(const std::filesystem::path& name, bool strip_filename = false);
bool iequals(const std::string& a, const std::string& b);
bool is_ziplike(const std::filesystem::path &fname);
std::filesystem::path home_directory();
std::string human_number(uint64_t value);
std::string format_time(const std::string &format, time_t timestamp);
void sort_strings_case_insensitive(std::vector<std::string> &strings);
std::string string_format(const char *format, ...) PRINTF_LIKE(1, 2);
std::string string_format_v(const char *format, va_list ap);
std::string slurp(const std::string &fname);
std::string pad_string(const std::string& string, size_t width, char c = ' ');
std::string pad_string_left(const std::string& string, size_t width, char c = ' ');
std::vector<std::string> slurp_lines(const std::string &file_name);
void write_lines(const std::string& file_name, const std::vector<std::string>& lines);

#endif
