/*
  zip_util.cc -- utility functions for zip needed only by ckmame itself
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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

#include "zip_util.h"

#include <filesystem>

#include "Exception.h"

ZipSource::~ZipSource() {
    zip_source_free(source);
}


void ZipSource::open() {
    if (zip_source_open(source) < 0) {
        throw Exception(error());
    }
}


void ZipSource::close() {
    if (zip_source_close(source) < 0) {
        throw Exception(error());
    }
}


uint64_t ZipSource::read(void *data, uint64_t length) {
    auto n = zip_source_read(source, data, length);

    if (n < 0) {
        throw Exception(error());
    }
    
    return static_cast<uint64_t>(n);
}


std::string ZipSource::error() {
    return zip_error_strerror(zip_source_error(source));
}


static bool my_zip_rename_to_unique(struct zip *za, zip_uint64_t idx);

int
my_zip_rename(struct zip *za, uint64_t idx, const char *name) {
    int zerr;
    zip_int64_t idx2;

    if (zip_rename(za, idx, name) == 0) {
	return 0;
    }

    zip_error_get(za, &zerr, NULL);

    if (zerr != ZIP_ER_EXISTS) {
	return -1;
    }

    idx2 = zip_name_locate(za, name, 0);
    if (idx2 == -1) {
	return -1;
    }
    if (!my_zip_rename_to_unique(za, (zip_uint64_t)idx2)) {
	return -1;
    }

    return zip_rename(za, idx, name);
}


static bool
my_zip_rename_to_unique(struct zip *za, zip_uint64_t idx) {

    std::string name = zip_get_name(za, idx, 0);
    if (name.empty()) {
	return false;
    }

    std::string ext = std::filesystem::path(name).extension();
    name.resize(name.length() - ext.length());

    for (int i = 0; i < 1000; i++) {
	char n[5];
	int zerr;

	sprintf(n, "-%03d", i);
	auto unique = name + n + ext;

	int ret = zip_rename(za, idx, unique.c_str());
	zip_error_get(za, &zerr, NULL);
	if (ret == 0 || zerr != ZIP_ER_EXISTS) {
	    return ret == 0;
	}
    }

    return false;
}
