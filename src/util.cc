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

#include <cinttypes>
#include <cstring>
#include <filesystem>
#include <vector>

#include "config.h"
#include "error.h"
#include "Exception.h"
#include "globals.h"


std::string rom_dir;

std::string
bin2hex(const uint8_t *data, size_t length) {
    char b[length * 2 + 1];

    for (size_t i = 0; i < length; i++) {
        sprintf(b + 2 * i, "%02x", data[i]);
    }
    b[2 * length] = '\0';

    return b;
}


#define HEX2BIN(c) (((c) >= '0' && (c) <= '9') ? (c) - '0' : ((c) >= 'A' && (c) <= 'F') ? (c) - 'A' + 10 : (c) - 'a' + 10)

int hex2bin(unsigned char *t, const char *s, size_t tlen) {
    if (strspn(s, "0123456789AaBbCcDdEeFf") != tlen * 2 || s[tlen * 2] != '\0') {
	return -1;
    }

    for (size_t i = 0; i < tlen; i++) {
	t[i] = static_cast<unsigned char>(HEX2BIN(s[i * 2]) << 4 | HEX2BIN(s[i * 2 + 1]));
    }
    
    return 0;
}


std::vector<uint8_t> hex2bin(const std::string &hex) {
    if (hex.size() % 2 != 0) {
        throw Exception("hex string with odd number of digits");
    }
    auto bin = std::vector<uint8_t>(hex.size() / 2);
    if (hex2bin(bin.data(), hex.c_str(), hex.size() / 2) != 0) {
        throw Exception("invalid hex string '" + hex + "'");
    }
    
    return bin;
}

name_type_t name_type(const std::string &name) {
    if (!std::filesystem::exists(name)) {
        return NAME_UNKNOWN;
    }

    if (std::filesystem::is_directory(name)) {
        if (roms_unzipped) {
            return NAME_ZIP;
        }
        else {
            return NAME_IMAGES;
        }
    }

    auto filename = std::filesystem::path(name).filename();
    if (filename == DBH_CACHE_DB_NAME || filename == ".DS_Store" || filename.string().substr(0, 2) == "._") {
        return NAME_IGNORE;
    }
    
    if (!roms_unzipped && is_ziplike(name)) {
	return NAME_ZIP;
    }

    return NAME_UNKNOWN;
}

bool
ensure_dir(const std::string &name, bool strip_filename) {
    std::error_code ec;
    std::string dir = name;

    if (strip_filename) {
	dir = std::filesystem::path(name).parent_path();
	if (dir == "") {
	    return true;
	}
    }

    std::filesystem::create_directories(dir, ec);

    if (ec) {
	myerror(ERRDEF, "cannot create '%s': %s", dir.c_str(), ec.message().c_str());
	return false;
    }

    return true;
}


const std::string get_directory(void) {
    if (!rom_dir.empty()) {
	return rom_dir;
    }
    return "roms";
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


void
print_human_number(FILE *f, uint64_t value) {
    if (value > 1024ul * 1024 * 1024 * 1024)
        printf("%" PRIi64 ".%02" PRIi64 "TiB", value / (1024ul * 1024 * 1024 * 1024), (((value / (1024ul * 1024 * 1024)) * 10 + 512) / 1024) % 100);
    else if (value > 1024 * 1024 * 1024)
        printf("%" PRIi64 ".%02" PRIi64 "GiB", value / (1024 * 1024 * 1024), (((value / (1024 * 1024)) * 10 + 512) / 1024) % 100);
    else if (value > 1024 * 1024)
        printf("%" PRIi64 ".%02" PRIi64 "MiB", value / (1024 * 1024), (((value / 1024) * 10 + 512) / 1024) % 100);
    else
        printf("%" PRIi64 " bytes", value);
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
    while (1) {
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
    return str;
}
