#ifndef HAD_FILE_H
#define HAD_FILE_H

/*
  File.h -- information about one file
  Copyright (C) 1999-2024 Dieter Baron and Thomas Klausner

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

#include "FileData.h"

class File : public FileData {
  public:
    File() : FileData(), broken(false) {}

    uint64_t get_size(size_t detector) const { return get_hashes(detector).size; }
    const Hashes& get_hashes(size_t detector) const;

    bool is_size_known(size_t detector) const { return get_size(detector) != Hashes::SIZE_UNKNOWN; }
    bool has_all_hashes(size_t detector, int requested_types) const;
    bool has_all_hashes(size_t detector, const Hashes& other) const {
        return has_all_hashes(detector, other.get_types());
    }
    bool size_hashes_are_set(size_t detector) const;

    std::string filename_extension;
    bool broken;

    std::unordered_map<size_t, Hashes> detector_hashes;

    std::string filename() const { return name + filename_extension; }

  private:
    static Hashes empty_hashes;
};

#endif // HAD_FILE_H
