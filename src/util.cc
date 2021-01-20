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

#include <filesystem>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "error.h"
#include "globals.h"
#include "util.h"

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

int
hex2bin(unsigned char *t, const char *s, size_t tlen) {
    unsigned int i;

    if (strspn(s, "0123456789AaBbCcDdEeFf") != tlen * 2 || s[tlen * 2] != '\0')
	return -1;

    for (i = 0; i < tlen; i++)
	t[i] = HEX2BIN(s[i * 2]) << 4 | HEX2BIN(s[i * 2 + 1]);

    return 0;
}


name_type_t
name_type(const std::string &name) {
    if (roms_unzipped) {
	if (!std::filesystem::exists(name)) {
	    return NAME_UNKNOWN;
	}
	if (std::filesystem::is_directory(name)) {
	    return NAME_ZIP;
	}
    }

    auto ext = std::filesystem::path(name).extension();
    if (ext == ".chd") {
	return NAME_CHD;
    }
    if (!roms_unzipped && strcasecmp(ext.c_str(), ".zip") == 0) {
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


std::string
status_name(status_t status, bool verbose) {
    switch (status) {
        case STATUS_OK:
            if (verbose) {
                return "ok";
            }
            else {
                return "";
            }

        case STATUS_BADDUMP:
            return "baddump";

        case STATUS_NODUMP:
            return "nodump";

        default:
            return "";
    }
}
