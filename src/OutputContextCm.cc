/*
  OutputContextCm.cc -- write games to clrmamepro dat files
  Copyright (C) 2006-2014 Dieter Baron and Thomas Klausner

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

#include "OutputContextCm.h"

#include <algorithm>
#include <cerrno>
#include <cinttypes>
#include <cstring>

#include "globals.h"
#include "util.h"


bool OutputContextCm::write_header(const DatEntry& dat) {
    stream << "clrmamepro (" << std::endl;
    cond_print_string("\tname ", dat.name, "\n");
    cond_print_string("\tdescription ", (dat.description.empty() ? dat.name : dat.description), "\n");
    cond_print_string("\tversion ", dat.version, "\n");
    stream << ")\n\n";

    return true;
}


bool OutputContextCm::write_game(const GamePtr game) {
    stream << "game (\n";
    cond_print_string("\tname ", game->name, "\n");
    cond_print_string("\tdescription ", game->description.empty() ? game->name : game->description, "\n");
    cond_print_string("\tcloneof ", game->cloneof[0], "\n");
    cond_print_string("\tromof ", game->cloneof[0], "\n");
    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        for (auto& rom : game->files[ft]) {
            stream << "\t" << (ft == TYPE_ROM ? "rom" : "disk") << " ( ";
            cond_print_string("name ", rom.name, " ");
            if (rom.where != FILE_INGAME) {
                cond_print_string("merge ", rom.merge.empty() ? rom.name : rom.merge, " ");
            }
            stream << "size " << rom.hashes.size << " ";
            cond_print_hash("crc ", Hashes::TYPE_CRC, &rom.hashes, " ");
            cond_print_hash("md5 ", Hashes::TYPE_MD5, &rom.hashes, " ");
            cond_print_hash("sha1 ", Hashes::TYPE_SHA1, &rom.hashes, " ");
            cond_print_string("flags ", rom.status_name(), " ");
            stream << ")\n";
        }
    }
    stream << ")\n\n";

    return true;
}
