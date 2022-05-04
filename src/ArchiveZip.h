#ifndef _HAD_ARCHIVE_ZIP_H
#define _HAD_ARCHIVE_ZIP_H

/*
  ArchiveZip.h -- archive for zip files
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

#include <zip.h>

#include <utility>

#include "Archive.h"

class ArchiveZip : public Archive {
public:
    ArchiveZip(const std::string &name, filetype_t filetype, where_t where, int flags) : Archive(ARCHIVE_ZIP, name, filetype, where, flags), za(nullptr) { }
    explicit ArchiveZip(ArchiveContentsPtr contents) : Archive(std::move(contents)), za(nullptr) { }

    ~ArchiveZip() override;

    bool check() override;
    bool close_xxx() override;
    bool commit_xxx() override;
    void commit_cleanup() override;
    void get_last_update() override;
    bool read_infos_xxx() override;

protected:
    zip_t *za;
    
    ZipSourcePtr get_source(uint64_t index, uint64_t start, std::optional<uint64_t> length) override;
    bool ensure_zip();
    
    bool ensure_file_doesnt_exist(const std::string &name);
};

#endif // _HAD_ARCHIVE_ZIP_H
