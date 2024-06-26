/*
  fix_util.cc -- utility functions needed only by ckmame itself
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

#include "fix_util.h"

#include <algorithm>
#include <cinttypes>
#include <filesystem>

#include "check_util.h"
#include "DeleteList.h"
#include "file_util.h"
#include "find.h"
#include "globals.h"
#include "RomDB.h"
#include "util.h"
#include "CkmameCache.h"


std::string
make_garbage_name(const std::string &name, int unique) {
    auto s = std::filesystem::path(name).filename();

    auto t = std::filesystem::path(configuration.unknown_directory) / s;
    
    if (unique && std::filesystem::exists(t)) {
	/* skip '.' */
	auto ext = s.extension();
	/* path and filename, but no extension */
	auto t_no_ext = t.parent_path() / t.stem();
	return make_unique_name(t_no_ext, ext);
    }

    return t;
}


static std::string
make_needed_name(filetype_t filetype, const FileData *r) {
    /* <configuration.saved_directory>/<crc>-nnn.zip */

    auto hash = r->hashes.to_string(filetype == TYPE_ROM ? Hashes::TYPE_CRC : Hashes::TYPE_SHA1);
    auto prefix = std::filesystem::path(configuration.saved_directory) / hash;

    return make_unique_name(prefix, (filetype == TYPE_ROM && configuration.roms_zipped) ? ".zip" : "");
}


int
move_image_to_garbage(const std::string &fname) {
    int ret;

    auto to_name = make_garbage_name(fname, 1);
    ensure_dir(to_name, true);
    ret = rename_or_move(fname, to_name);

    return ret ? 0 : -1;
}


void remove_empty_archive(Archive *archive) {
    bool quiet = archive->contents->flags & ARCHIVE_FL_TOP_LEVEL_ONLY;
    
    if (!quiet) {
	output.message_verbose("remove empty archive");
    }
    if (ckmame_cache->superfluous_delete_list) {
	ckmame_cache->superfluous_delete_list->remove_archive(archive);
    }
}


bool save_needed_part(Archive *sa, size_t sidx, const std::string &gamename, uint64_t start, std::optional<uint64_t> length, FileData *f) {
    bool needed = true;

    if (!sa->file_ensure_hashes(sidx, db->hashtypes(sa->filetype))) {
        return false;
    }
    
    if (find_in_romset(sa->filetype, 0, f, sa, gamename, "", nullptr) == FIND_EXISTS) {
        needed = false;
    }
    else {
	ckmame_cache->ensure_needed_maps();
        if (find_in_archives(sa->filetype, 0, f, nullptr, true) == FIND_EXISTS) {
            needed = false;
        }
    }
    
    if (needed) {
	if (!length.has_value()) {
	    output.message_verbose("save needed file '%s'", sa->files[sidx].filename().c_str());
	}
	else {
	    output.message_verbose("extract (offset %" PRIu64 ", size %" PRIu64 ") from '%s' to needed", start, length.value(), sa->files[sidx].filename().c_str());
	}

        auto tmp = make_needed_name(sa->filetype, f);
        if (tmp.empty()) {
            output.error("cannot create needed file name");
            return false;
        }
        
        ArchivePtr da  = Archive::open(tmp, sa->filetype, FILE_NEEDED, ARCHIVE_FL_CREATE | (configuration.fix_romset ? 0 : ARCHIVE_FL_RDONLY));
        
        if (!da) {
            return false;
        }
        
        if (!da->file_copy_part(sa, sidx, sa->files[sidx].name, start, length, &f->hashes) || !da->commit()) {
            da->rollback();
            return false;
        }
    }
    else {
        if (!length.has_value()) {
	    output.message_verbose("delete unneeded file '%s'", sa->files[sidx].filename().c_str());
        }
    }
    
    if (configuration.fix_romset && !length.has_value()) {
        if (sa->where == FILE_ROMSET) {
            return sa->file_delete(sidx);
        }
        else {
            ckmame_cache->used(sa, sidx);
        }
    }
    
    return true;
}

bool
save_needed(Archive *sa, size_t sidx, const std::string &gamename) {
    return save_needed_part(sa, sidx, gamename, 0, {}, &sa->files[sidx]);
}
