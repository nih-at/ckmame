/*
  fix_util.c -- utility functions needed only by ckmame itself
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
#include <filesystem>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "check_util.h"
#include "error.h"
#include "Exception.h"
#include "file_util.h"
#include "find.h"
#include "globals.h"
#include "util.h"


std::string
make_garbage_name(const std::string &name, int unique) {
    auto s = std::filesystem::path(name).filename();

    auto t = std::filesystem::path(unknown_dir) / s;
    
    if (unique && std::filesystem::exists(t)) {
	/* skip '.' */
	auto ext = s.extension();
	/* path and filename, but no extension */
	auto t_no_ext = t.parent_path() / t.stem();
	return make_unique_name(t_no_ext, ext);
    }

    return t;
}


std::string
make_unique_name(const std::string &prefix, const std::string &ext) {
    char buf[5];

    for (int i = 0; i < 1000; i++) {
	std::error_code ec;
	sprintf(buf, "-%03d", i);
	auto testname = prefix + buf + ext;
	if (std::filesystem::exists(testname, ec)) {
	    continue;
	}
	if (ec) {
	    continue;
	}
	return testname;
    }

    throw Exception("can't create unique file name"); // TODO: details?
}


static std::string
make_needed_name(filetype_t filetype, const File *r) {
    /* <needed_dir>/<crc>-nnn.zip */

    auto hash = r->hashes.to_string(filetype == TYPE_ROM ? Hashes::TYPE_CRC : Hashes::TYPE_SHA1);
    auto prefix = std::filesystem::path(needed_dir) / hash;

    return make_unique_name(prefix, (filetype == TYPE_ROM && !roms_unzipped) ? ".zip" : "");
}


int
move_image_to_garbage(const std::string &fname) {
    int ret;

    auto to_name = make_garbage_name(fname, 1);
    ensure_dir(to_name, true);
    ret = rename_or_move(fname, to_name);

    return ret ? 0 : -1;
}


void remove_empty_archive(const std::string &name, bool quiet) {
    if ((fix_options & FIX_PRINT) && !quiet) {
	printf("%s: remove empty archive\n", name.c_str());
    }
    remove_from_superfluous(name);
}


void
remove_from_superfluous(const std::string &name) {
    auto entry = std::find(superfluous.begin(), superfluous.end(), name);
    if (entry != superfluous.end()) {
	/* "needed" zip archives are not in list */
	superfluous.erase(entry);
    }
}


bool
save_needed_part(Archive *sa, size_t sidx, const std::string &gamename, off_t start, off_t length, File *f) {
    bool do_save = fix_options & FIX_DO;

    bool needed = true;

    if (!sa->file_ensure_hashes(sidx, db->hashtypes(TYPE_ROM))) {
        return false;
    }
    
    if (find_in_romset(sa->filetype, f, sa, gamename, NULL) == FIND_EXISTS) {
        needed = false;
    }
    else {
        ensure_needed_maps();
        if (find_in_archives(sa->filetype, f, NULL, true) == FIND_EXISTS) {
            needed = false;
        }
    }
    
    if (needed) {
        if (fix_options & FIX_PRINT) {
            if (length == -1) {
                printf("%s: save needed file '%s'\n", sa->name.c_str(), sa->files[sidx].filename().c_str());
            }
            else {
                printf("%s: extract (offset %" PRIu64 ", size %" PRIu64 ") from '%s' to needed\n", sa->name.c_str(), (uint64_t)start, (uint64_t)length, sa->files[sidx].filename().c_str());
            }
        }
        
        auto tmp = make_needed_name(sa->filetype, f);
        if (tmp.empty()) {
            myerror(ERRDEF, "cannot create needed file name");
            return false;
        }
        
        ArchivePtr da  = Archive::open(tmp, sa->filetype, FILE_NEEDED, ARCHIVE_FL_CREATE | (do_save ? 0 : ARCHIVE_FL_RDONLY));
        
        if (!da) {
            return false;
        }
        
        if (!da->file_copy_part(sa, sidx, sa->files[sidx].name, start, length == -1 ? std::optional<uint64_t>() : length, f) || !da->commit()) {
            da->rollback();
            return false;
        }
    }
    else {
        if (length == -1 && (fix_options & FIX_PRINT)) {
            printf("%s: delete unneeded file '%s'\n", sa->name.c_str(), sa->files[sidx].filename().c_str());
        }
    }
    
    if (do_save && length == -1) {
        if (sa->where == FILE_ROMSET) {
            return sa->file_delete(sidx);
        }
        else {
            DeleteList::used(sa, sidx);
        }
    }
    
    return true;
}

bool
save_needed(Archive *sa, size_t sidx, const std::string &gamename) {
    return save_needed_part(sa, sidx, gamename, 0, -1, &sa->files[sidx]);
}
