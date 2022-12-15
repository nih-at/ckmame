/*
  warn.h -- emit warning
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

#include "warn.h"
#include "globals.h"
#include "util.h"

#include <cinttypes>


void
warn_archive_file(filetype_t ft, const File *r, const std::string &reason) {
    switch (ft) {
        case TYPE_ROM:
	    output.message("file %-12s  size %7" PRIu64 "  crc %.8" PRIx32 ": %s", r->filename().c_str(), r->hashes.size, r->hashes.crc, reason.c_str());
            break;
            
        case TYPE_DISK:
	    output.message("image %-12s: %s", r->filename().c_str(), reason.c_str());
            break;
            
        default:
            break;
    }
}


void warn_game(filetype_t ft, const Game* game, const std::string& reason) {
    output.message("%s: %s", pad_string("game " + game->name, 45).c_str(), reason.c_str());
}


void warn_game_file(filetype_t ft, const Rom *r, const std::string &reason) {
    std::string message;
    switch (ft) {
    case TYPE_ROM:
	message += "rom  " + pad_string(r->name, 12) + "  ";
	if (r->is_size_known()) {
	    message += "size " + pad_string_left(std::to_string(r->hashes.size), 7);

            switch (r->status) {
            case Rom::OK:
                if (r->hashes.has_type(Hashes::TYPE_CRC)) {
                    message += "  crc " + r->hashes.to_string(Hashes::TYPE_CRC);
                }
                break;
            case Rom::BAD_DUMP:
                message += "  bad dump";
                break;
            case Rom::NO_DUMP:
                message += "  no good dump";
            }
	}
	break;

    case TYPE_DISK: {
	message += "disk " + pad_string(r->name, 12) + "  ";

	auto &h = r->hashes;
	if (h.has_type(Hashes::TYPE_SHA1)) {
	    message += "sha1 " + h.to_string(Hashes::TYPE_SHA1);
	}
	else if (h.has_type(Hashes::TYPE_MD5)) {
	    message += "md5 " + h.to_string(Hashes::TYPE_MD5);
	}
	else {
	    message += "no good dump";
	}
	break;
    }

    default:
	break;
    }

    output.message("%s: %s", pad_string(message, 45).c_str(), reason.c_str());
}


void
warn_set_info(warn_type_t type, const std::string &name) {
    /* keep in sync with warn_type_t in warn.h */
    static std::string tname[] = {"archive", "game", "image"};

    auto message = "In " + tname[type] + " " + name;

    if (type == WARN_TYPE_ARCHIVE && name[name.length() - 1] == '/') {
	message.pop_back();
    }
    output.set_subheader(message);
}


void warn_unset_info() {
    output.set_subheader("");
}
