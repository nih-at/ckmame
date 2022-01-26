/*
  garbage.c -- move files to garbage directory
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

#include "Garbage.h"

#include "fix_util.h"
#include "util.h"


bool Garbage::add(uint64_t index, bool copy) {
    if (!open()) {
	return false;
    }

    std::string source_name = sa->files[index].name;
    std::string destination_name = source_name;
    
    if (da->file_index_by_name(source_name) >= 0) {
        destination_name = da->make_unique_name_in_archive(source_name);
    }

    return da->file_copy_or_move(sa, index, destination_name, copy);
}


bool Garbage::close() {
    if (!da) {
	return true;
    }

    if (!da->is_empty()) {
        if (!ensure_dir(da->name, true)) {
	    return false;
        }
    }

    if (!da->close()) {
	return false;
    }
    
    return true;
}


bool Garbage::commit() {
    if (!da) {
        return true;
    }

    return da->commit();
}


bool Garbage::open() {
    if (!opened) {
        opened = true;
	auto name = make_garbage_name(sa->name, 0);
        da = Archive::open(name, sa->contents->filetype, FILE_NOWHERE, ARCHIVE_FL_CREATE);
        if (!da->check()) {
            da = nullptr;
	}
    }

    return !!da;
}
