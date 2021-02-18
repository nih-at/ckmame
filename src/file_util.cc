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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "error.h"
#include "hashes.h"

bool
link_or_copy(const std::string &old, const std::string &new_name) {
    std::error_code ec;
    std::filesystem::create_hard_link(old, new_name, ec);
    if (ec) {
	if (!std::filesystem::copy_file(old, new_name, ec) || ec) {
	    seterrinfo(old);
	    myerror(ERRFILESTR, "cannot copy to '%s'", new_name.c_str());
	    return false;
	}
    }

    return true;;
}


bool
my_remove(const std::string &name) {
    std::error_code ec;
    std::filesystem::remove(name, ec);
    if (ec) {
	seterrinfo(name);
	myerror(ERRFILE, "cannot remove: %s", ec.message().c_str());
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
	    seterrinfo(old);
	    myerror(ERRFILESTR, "cannot rename to '%s'", new_name.c_str());
	    return false;
	}
	std::filesystem::remove(old);
    }

    return true;
}
