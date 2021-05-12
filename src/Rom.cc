
/*
  ROM.cc -- information about one ROM
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

#include "Rom.h"

bool Rom::compare_merged(const FileData &other) const {
    return merged_name() == other.name;
}


bool Rom::compare_merged(const Rom &other) const {
    return merged_name() == other.merged_name();
}


std::string Rom::filename(filetype_t filetype) const {
    if (filetype == TYPE_DISK) {
        return name + ".chd";
    }
    else {
        return name;
    }
}


bool Rom::is_mergable(const Rom &other) const {
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


Hashes::Compare FileData::compare_hashes(const FileData &other) const {
    auto result = hashes.compare(other.hashes);
    
    if (result == Hashes::MATCH) {
        return result;
    }
    
#if 0
    // move to version with detector id we're interested in
    if (!hashes.empty() && !other.hashes_detector.empty() && hashes.compare(other.hashes_detector) == Hashes::MATCH) {
        return Hashes::MATCH;
    }
#endif
    
    return result;
}
