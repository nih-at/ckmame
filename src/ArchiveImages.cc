/*
  ArchiveImages.cc -- archive for directory of disk images
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

#include "ArchiveImages.h"

#include <filesystem>
#include <cstring>
#include <sys/stat.h>

#include "Chd.h"
#include "Dir.h"
#include "Exception.h"
#include "globals.h"
#include "util.h"

ArchiveImages::ArchiveImages(const std::string &name, filetype_t filetype, where_t where, int flags) : ArchiveDir(name, filetype, where, flags) {
    contents->archive_type = ARCHIVE_IMAGES;
    contents->filename_extension = ".chd";
}


ZipSourcePtr ArchiveImages::Archive::get_source(uint64_t index, uint64_t start, std::optional<uint64_t> length) {
    output.set_error_archive(name);
    throw Exception("cannot open '%s': reading from CHDs not supported", files[index].name.c_str());
}


bool ArchiveImages::read_infos_xxx() {
    if (!configuration.roms_zipped) {
        return true;
    }
    
    try {
        Dir dir(name, (contents->flags & ARCHIVE_FL_TOP_LEVEL_ONLY) == 0);
        std::filesystem::path filepath;
        
        while ((filepath = dir.next()) != "") {
            if (name == filepath || name_type(filepath) == NAME_IGNORE || filepath.extension() != ".chd" || !std::filesystem::is_regular_file(filepath)) {
                continue;
            }
            
            files.emplace_back();
            auto &f = files[files.size() - 1];
            auto filename = filepath.string();
            auto start = name.length() + 1;
            f.name = filename.substr(start, filename.length() - start - 4);

            try {
                struct stat sb;
                
                if (stat(filepath.c_str(), &sb) != 0) {
                    throw Exception("%s", strerror(errno));
                }

                // auto ftime = std::filesystem::last_write_time(filepath);
                // f.mtime = decltype(ftime)::clock::to_time_t(ftime);
                f.mtime = sb.st_mtime;

                Chd chd(filepath);
                
                f.hashes.size = chd.size();
                f.hashes.set_hashes(chd.hashes);
            }
            catch (Exception &e) {
                output.error("%s: can't open: %s", filename.c_str(), e.what());
                f.broken = true;
            }
        }
    }
    catch (...) {
        return false;
    }
    
    return true;
}
