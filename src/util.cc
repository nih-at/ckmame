/*
  util.c -- utility functions
  Copyright (C) 1999-2018 Dieter Baron and Thomas Klausner

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

#include "util.h"

#include <cctype>
#include <cinttypes>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include "CkmameDB.h"
#include "config.h"
#include "DatDb.h"
#include "Exception.h"
#include "globals.h"


#define BIN2HEX(n) ((n) >= 10 ? (n) + 'a' - 10 : (n) + '0')


std::string bin2hex(const std::vector<uint8_t> &bin) {
    auto hex = std::string(bin.size() * 2, '\0');
    
    for (size_t i = 0; i < bin.size(); i++) {
        hex[i * 2] = BIN2HEX(bin[i] >> 4);
        hex[i * 2 + 1] = BIN2HEX(bin[i] & 0xf);
    }
    
    return hex;
}


#define HEX2BIN(c) (((c) >= '0' && (c) <= '9') ? (c) - '0' : ((c) >= 'A' && (c) <= 'F') ? (c) - 'A' + 10 : (c) - 'a' + 10)


std::vector<uint8_t> hex2bin(const std::string &hex) {
    if (hex.size() % 2 != 0) {
        throw Exception("hex string with odd number of digits");
    }
    
    if (hex.find_first_not_of("0123456789AaBbCcDdEeFf") != std::string::npos) {
        throw Exception("hex string with invalid digit");
    }
    
    auto bin = std::vector<uint8_t>(hex.size() / 2);
    
    for (size_t i = 0; i < bin.size(); i++) {
        bin[i] = static_cast<uint8_t>(HEX2BIN(hex[i * 2]) << 4 | HEX2BIN(hex[i * 2 + 1]));
    }
    
    return bin;
}

name_type_t name_type(const std::string &name) {
    if (!std::filesystem::exists(name)) {
        return NAME_UNKNOWN;
    }

    if (std::filesystem::is_directory(name)) {
        if (configuration.roms_zipped) {
            return NAME_IMAGES;
        }
        else {
            return NAME_ZIP;
        }
    }

    auto filename = std::filesystem::path(name).filename();
    if (filename == CkmameDB::db_name || filename == DatDB::db_name || filename == ".DS_Store" || filename.string().substr(0, 2) == "._") {
        return NAME_IGNORE;
    }
    
    if (configuration.roms_zipped && is_ziplike(name)) {
	return NAME_ZIP;
    }

    return NAME_UNKNOWN;
}


bool ensure_dir(const std::filesystem::path& name, bool strip_filename) {
    try {
        ensure_directory(name, strip_filename);
    }
    catch (Exception &ex) {
        output.error("%s", ex.what());
        return false;
    }

    return true;
}


void ensure_directory(const std::filesystem::path& name, bool strip_filename) {
    std::error_code ec;
    auto dir = strip_filename ? name.parent_path() : name;

    if (dir.empty()) {
        return;
    }

    std::filesystem::create_directories(dir, ec);

    if (ec) {
        throw Exception("cannot create '%s': %s", dir.c_str(), ec.message().c_str());
    }
}


std::filesystem::path home_directory() {
    auto home = getenv("HOME");
    if (home == nullptr) {
        return "";
    }

    return home;
}


bool is_ziplike(const std::string &fname) {
    auto extension = std::filesystem::path(fname).extension();
    
    if (strcasecmp(extension.c_str(), ".zip") == 0) {
        return true;
    }
    
#ifdef HAVE_LIBARCHIVE
    if (strcasecmp(extension.c_str(), ".7z") == 0) {
        return true;
    }
#endif
    
    return false;
}


std::string human_number(uint64_t value) {
    char s[128];
    if (value > 1024ul * 1024 * 1024 * 1024) {
	sprintf(s, "%" PRIi64 ".%02" PRIi64 "TiB", value / (1024ul * 1024 * 1024 * 1024), (((value / (1024ul * 1024 * 1024)) * 10 + 512) / 1024) % 100);
    }
    else if (value > 1024 * 1024 * 1024) {
	sprintf(s, "%" PRIi64 ".%02" PRIi64 "GiB", value / (1024 * 1024 * 1024), (((value / (1024 * 1024)) * 10 + 512) / 1024) % 100);
    }
    else if (value > 1024 * 1024) {
	sprintf(s, "%" PRIi64 ".%02" PRIi64 "MiB", value / (1024 * 1024), (((value / 1024) * 10 + 512) / 1024) % 100);
    }
    else {
	sprintf(s, "%" PRIi64 " bytes", value);
    }

    return s;
}


std::string string_format(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    auto string = string_format_v(format, ap);
    va_end(ap);
    return string;
}

std::string string_format_v(const char *format, va_list ap) {
    auto size = strlen(format) + 50;
    std::string str;
    va_list ap2;
    while (true) {
        str.resize(size);
        va_copy(ap2, ap);
        int n = vsnprintf((char *)str.data(), size, format, ap2);
        va_end(ap2);
        if (n > -1 && static_cast<size_t>(n) < size) {
            str.resize(static_cast<size_t>(n));
            return str;
        }
        if (n > -1) {
            size = static_cast<size_t>(n) + 1;
        }
        else {
            size *= 2;
        }
    }
}


std::string string_lower(const std::string &s) {
    auto l = std::string(s.size(), ' ');
    
    for (size_t i = 0; i < s.size(); i++) {
        l[i] = static_cast<char>(tolower(s[i]));
    }
    
    return l;
}



std::string slurp(const std::string &filename) {
    auto f = std::ifstream(filename, std::ios::in | std::ios::binary);
        
    const auto size = std::filesystem::file_size(filename);
        
    std::string text(size, '\0');
        
    f.read(text.data(), static_cast<std::streamsize>(size));
        
    return text;
}


std::string format_time(const std::string &format, time_t timestamp) {
    char string[100];
    auto tm = localtime(&timestamp);

    strftime(string, sizeof(string), format.c_str(), tm);

    return string;
}


std::vector<std::string> slurp_lines(const std::string &file_name) {
    auto file = std::ifstream(file_name, std::ios::in);

    auto lines = std::vector<std::string>();
    auto line = std::string();

    while (getline(file, line)) {
	lines.push_back(line);
    }

    return lines;
}


bool string_starts_with(const std::string &large, const std::string &small) {
    return large.rfind(small, 0) != std::string::npos;
}


std::string pad_string(const std::string &string, size_t width, char c) {
    size_t length = string.length();

    if (length >= width) {
	return string;
    }
    return string + std::string(width - length, c);
}

std::string pad_string_left(const std::string &string, size_t width, char c) {
    size_t length = string.length();

    if (length >= width) {
	return string;
    }
    return std::string(width - length, c) + string;
}
