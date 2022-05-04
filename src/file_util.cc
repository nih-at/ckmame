/*
  file_util.cc -- utility functions for manipulating files
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

#include "file_util.h"

#include <filesystem>

#include "Exception.h"
#include "globals.h"

bool
link_or_copy(const std::string &old, const std::string &new_name) {
    std::error_code ec;
    std::filesystem::create_hard_link(old, new_name, ec);
    if (ec) {
	if (!std::filesystem::copy_file(old, new_name, ec) || ec) {
	    output.error_error_code(ec, "cannot copy '%s' to '%s'", old.c_str(), new_name.c_str());
	    return false;
	}
    }

    return true;
}


bool
my_remove(const std::string &name) {
    std::error_code ec;
    std::filesystem::remove(name, ec);
    if (ec) {
	output.error_error_code(ec, "cannot remove '%s'", name.c_str());
	return false;
    }

    return true;
}


bool rename_or_move(const std::string &old, const std::string &new_name) {
    std::error_code ec;
    std::filesystem::rename(old, new_name, ec);
    if (ec) {
        std::filesystem::copy_file(old, new_name, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
	    output.error_error_code(ec, "cannot rename '%s' to '%s'", old.c_str(), new_name.c_str());
	    return false;
	}
	std::filesystem::remove(old);
    }

    return true;
}


void remove_directories_up_to(const std::filesystem::path &directory, const std::filesystem::path &top) {
    std::error_code ec;

    auto dir = directory;
    while (true) {
        std::filesystem::remove(dir, ec);
        if (ec) {
            return;
        }

        if (dir == top) {
            break;
        }
        dir = dir.parent_path();
    }
}


std::string make_unique_name(const std::string &prefix, const std::string &ext) {
    char buf[5];

    for (int i = 0; i < 1000; i++) {
    std::error_code ec;
    sprintf(buf, "-%03d", i);
    auto testname = prefix + buf + ext;
    if (std::filesystem::exists(testname, ec)) {
        continue;
    }
    if (ec) {
        continue;
    }
    return testname;
    }

    throw Exception("can't create unique file name"); // TODO: details?
}


std::filesystem::path make_unique_path(const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) {
        return path;
    }
    
    auto extension = path.extension();
    auto prefix = path.parent_path() / path.stem();
    
    return make_unique_name(prefix, extension);
}
