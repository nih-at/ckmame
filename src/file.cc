/*
  file.c -- initialize / finalize file structure
  Copyright (C) 2004-2014 Dieter Baron and Thomas Klausner

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


#include <stdlib.h>

#include "file.h"


bool File::compare_merged(const File &other) const {
    return merged_name() == other.merged_name();
}


bool File::compare_name(const File &other) const {
    return name == other.name;
}

bool File::compare_name_size_hashes(const File &other) const {
    return compare_name(other) && compare_size_hashes(other);
}

bool File::compare_size_hashes(const File &other) const {
    return compare_size_hashes_one(other, false) || compare_size_hashes_one(other, true);
}

bool File::compare_size_hashes_one(const File &other, bool detector) const {
    if (detector && !other.size_hashes_are_set(detector)) {
        return false;
    }
    
    if (is_size_known() && other.is_size_known(detector) && size != other.get_size(detector)) {
        return false;
    }
    
    if (hashes.compare(other.get_hashes(detector)) == Hashes::MATCH) {
        return true;
    }
    
    return false;
}


bool File::size_hashes_are_set(bool detector) const {
    return is_size_known(detector) && !get_hashes(detector).empty();
}

bool File::is_mergable(const File &other) const {
    /* name must be the (merged) name */
    if (merged_name() != other.name) {
        return false;
    }

    /* Both can be bad dumps or the hashes must match. */
    if (hashes.empty() != other.hashes.empty()) {
        return false;
    }
    if (compare_size_hashes(other)) {
        return true;
    }

    return false;
}


Hashes::Compare File::compare_hashes(const File &other) const {
    auto result = hashes.compare(other.hashes);
    
    if (result == Hashes::MATCH) {
        return result;
    }
    
    if (!hashes.empty() && !other.hashes_detector.empty() && hashes.compare(other.hashes_detector) == Hashes::MATCH) {
        return Hashes::MATCH;
    }
    
    return result;
}
