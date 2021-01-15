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

#include <algorithm>
#include <filesystem>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "error.h"
#include "find.h"
#include "funcs.h"
#include "globals.h"
#include "util.h"
#include "xmalloc.h"


#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif


std::string
make_garbage_name(const std::string &name, int unique) {
    auto s = std::filesystem::path(name).filename();

    auto t = std::filesystem::path(unknown_dir) / s;
    
    if (unique && std::filesystem::exists(t)) {
	/* skip '.' */
	auto ext = s.extension().string().substr(1);
	/* path and filename, but no extension */
	auto t_no_ext = t.parent_path() / t.stem();
	auto u = make_unique_name(ext.c_str(), "%s", t_no_ext.c_str());
	return u;
    }

    return t;
}


char *
make_unique_name(const char *ext, const char *fmt, ...) {
    char ret[MAXPATHLEN];
    int i, len;
    struct stat st;
    va_list ap;

    va_start(ap, fmt);
    len = vsnprintf(ret, sizeof(ret), fmt, ap);
    va_end(ap);

    /* already used space, "-XXX.", extension, 0 */
    if (len + 5 + strlen(ext) + 1 > sizeof(ret)) {
	return NULL;
    }

    for (i = 0; i < 1000; i++) {
	sprintf(ret + len, "-%03d%s%s", i, (ext[0] ? "." : ""), ext);

	if (stat(ret, &st) == -1 && errno == ENOENT)
	    return xstrdup(ret);
    }

    return NULL;
}


char *
make_needed_name(const File *r) {
    /* <needed_dir>/<crc>-nnn.zip */

    auto crc = r->hashes.to_string(Hashes::TYPE_CRC);

    return make_unique_name(roms_unzipped ? "" : "zip", "%s/%s", needed_dir, crc.c_str());
}


char *
make_needed_name_disk(const Disk *d) {
    /* <needed_dir>/<md5>-nnn.zip */

    auto md5 = d->hashes.to_string(Hashes::TYPE_MD5);

    return make_unique_name("chd", "%s/%s", needed_dir, md5.c_str());
}


int
move_image_to_garbage(const std::string &fname) {
    int ret;

    auto to_name = make_garbage_name(fname, 1);
    ensure_dir(to_name, true);
    ret = rename_or_move(fname.c_str(), to_name.c_str());

    return ret;
}


void
remove_empty_archive(const std::string &name) {
    if (fix_options & FIX_PRINT) {
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
save_needed_part(Archive *sa, size_t sidx, const char *gamename, off_t start, off_t length, File *f) {
    char *tmp;
    bool do_save = fix_options & FIX_DO;

    bool needed = true;

    if (!sa->file_compute_hashes(sidx, romdb_hashtypes(db, TYPE_ROM))) {
        return false;
    }
    
    if (find_in_romset(f, sa, gamename, NULL) == FIND_EXISTS) {
	needed = false;	
    }
    else {
	ensure_needed_maps();
	if (find_in_archives(f, NULL, true) == FIND_EXISTS) {
	    needed = false;
	}
    }

    if (needed) {
	if (fix_options & FIX_PRINT) {
	    if (length == -1) {
		printf("%s: save needed file '%s'\n", sa->name.c_str(), sa->files[sidx].name.c_str());
	    }
	    else {
                printf("%s: extract (offset %" PRIu64 ", size %" PRIu64 ") from '%s' to needed\n", sa->name.c_str(), (uint64_t)start, (uint64_t)length, sa->files[sidx].name.c_str());
	    }
	}

	if ((tmp = make_needed_name(f)) == NULL) {
	    myerror(ERRDEF, "cannot create needed file name");
            return false;
	}
	
        ArchivePtr da  = Archive::open(tmp, sa->filetype, FILE_NEEDED, ARCHIVE_FL_CREATE | (do_save ? 0 : ARCHIVE_FL_RDONLY));

        if (!da) {
            return false;
        }
	
	free(tmp);
	
        if (!da->file_copy_part(sa, sidx, sa->files[sidx].name.c_str(), start, length == -1 ? std::optional<uint64_t>() : length, f) || !da->commit()) {
            da->rollback();
            return false;
        }
    }
    else {
	if (length == -1 && (fix_options & FIX_PRINT)) {
            printf("%s: delete unneeded file '%s'\n", sa->name.c_str(), sa->files[sidx].name.c_str());
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
save_needed(Archive *sa, size_t sidx, const char *gamename) {
    return save_needed_part(sa, sidx, gamename, 0, -1, &sa->files[sidx]);
}


int
save_needed_disk(const char *fname, int do_save) {
    DiskPtr d = Disk::from_file(fname, 0);
    if (!d) {
	return -1;
    }

    int ret = 0;

    if (do_save) {
        auto tmp = make_needed_name_disk(d.get());
	if (tmp == NULL) {
	    myerror(ERRDEF, "cannot create needed file name");
	    ret = -1;
	}
	else if (!ensure_dir(tmp, true)) {
	    ret = -1;
	}
	else if (rename_or_move(fname, tmp) != 0) {
	    ret = -1;
	}
	else {
            d = Disk::from_file(tmp, 0);
	}
        free(tmp);
    }

    ensure_needed_maps();
    enter_disk_in_map(d.get(), FILE_NEEDED);
    return ret;
}
