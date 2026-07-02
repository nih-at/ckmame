/*
  OutputContextMtree.cc -- write games to mtree(8) files
  Copyright (C) 2013-2014 Dieter Baron and Thomas Klausner

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

#include "OutputContextMtree.h"

#include <cerrno>
#include <cinttypes>
#include <cstring>

#include "globals.h"


static std::string strsvis_cstyle(const std::string& in) {
    /* maximal extension = 2/char */
    auto out = std::string(2 * in.length(), 0);

    size_t outpos = 0;
    for (auto c : in) {
        switch (c) {
        case '\007':
            out[outpos++] = '\\';
            out[outpos++] = 'a';
            break;
        case '\010':
            out[outpos++] = '\\';
            out[outpos++] = 'b';
            break;
        case '\014':
            out[outpos++] = '\\';
            out[outpos++] = 'f';
            break;
        case '\012':
            out[outpos++] = '\\';
            out[outpos++] = 'n';
            break;
        case '\015':
            out[outpos++] = '\\';
            out[outpos++] = 'r';
            break;
        case '\040':
            out[outpos++] = '\\';
            out[outpos++] = 's';
            break;
        case '\011':
            out[outpos++] = '\\';
            out[outpos++] = 't';
            break;
        case '\013':
            out[outpos++] = '\\';
            out[outpos++] = 'v';
            break;
        case '#':
            out[outpos++] = '\\';
            out[outpos++] = '#';
            break;
        default:
            out[outpos++] = c;
            break;
        }
    }
    out.resize(outpos);

    return out;
}


bool OutputContextMtree::write_game(const GamePtr game) {
    auto dirname = strsvis_cstyle(game->name);

    if (runtest) {
        for (size_t ft = 0; ft < TYPE_MAX; ft++) {
            if (game->files[ft].empty()) {
                continue;
            }
            stream << "./" << dirname << " type=dir\n";
            write_files(dirname, game->files[ft]);
        }
    }
    else {
        stream << "./" << dirname << " type=dir\n";
        for (size_t ft = 0; ft < TYPE_MAX; ft++) {
            write_files(dirname, game->files[ft]);
        }
    }
    return true;
}


bool OutputContextMtree::write_header(const DatEntry& dat) {
    stream << ". type=dir\n";

    return true;
}


void OutputContextMtree::write_files(const std::string& dirname, const std::vector<Rom>& files) {
    for (auto& file : files) {
        stream << "./" << dirname << "/" << strsvis_cstyle(file.name) << " type=file";
        /* For disks, this is the internal size and checksums and not information of the file on-disk. Disks are only
         * supported in zipped mode, where the mtree file can not be taken literally anyway, so this is ok. */
        if (file.is_size_known()) {
            stream << " size=" << file.hashes.size;
        }
        cond_print_hash(" sha256=", Hashes::TYPE_SHA256, &file.hashes, "");
        cond_print_hash(" sha1=", Hashes::TYPE_SHA1, &file.hashes, "");
        cond_print_hash(" md5=", Hashes::TYPE_MD5, &file.hashes, "");
        cond_print_string(" status=", file.status_name(), "");
        if (runtest) {
            cond_print_hash(" crc=", Hashes::TYPE_CRC, &file.hashes, "");
            stream << " time=" << static_cast<unsigned long long>(file.mtime);
        }
        stream << "\n";
    }
}
