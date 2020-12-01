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


char *
make_garbage_name(const char *name, int unique) {
    const char *s;
    char *t, *u, *ext;
    struct stat st;

    s = mybasename(name);

    t = (char *)xmalloc(strlen(unknown_dir) + strlen(s) + 2);

    sprintf(t, "%s/%s", unknown_dir, s);

    if (unique && (stat(t, &st) == 0 || errno != ENOENT)) {
	ext = strchr(t, '.');
	if (ext)
	    *(ext++) = '\0';
	u = make_unique_name(ext ? ext : "", "%s", t);
	free(t);
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
make_needed_name_disk(const disk_t *d) {
    /* <needed_dir>/<md5>-nnn.zip */

    auto md5 = d->hashes.to_string(Hashes::TYPE_MD5);

    return make_unique_name("chd", "%s/%s", needed_dir, md5.c_str());
}


int
move_image_to_garbage(const char *fname) {
    char *to_name;
    int ret;

    to_name = make_garbage_name(fname, 1);
    ensure_dir(to_name, 1);
    ret = rename_or_move(fname, to_name);
    free(to_name);

    return ret;
}


void
remove_empty_archive(const char *name) {
    int idx;

    if (fix_options & FIX_PRINT)
	printf("%s: remove empty archive\n", name);
    if (superfluous) {
	idx = parray_find(superfluous, name, reinterpret_cast<int (*)(const void *, const void *)>(strcmp));
	/* "needed" zip archives are not in list */
	if (idx >= 0)
	    parray_delete(superfluous, idx, free);
    }
}


void
remove_from_superfluous(const char *name) {
    int idx;

    if (superfluous) {
	idx = parray_find(superfluous, name, reinterpret_cast<int (*)(const void *, const void *)>(strcmp));
	/* "needed" images are not in list */
	if (idx >= 0)
	    parray_delete(superfluous, idx, free);
    }
}


bool
save_needed_part(Archive *sa, int sidx, const char *gamename, off_t start, off_t length, File *f) {
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
	    delete_list_used(sa, sidx);
	}
    }

    return true;
}

bool
save_needed(Archive *sa, int sidx, const char *gamename) {
    return save_needed_part(sa, sidx, gamename, 0, -1, &sa->files[sidx]);
}


int
save_needed_disk(const char *fname, int do_save) {
    char *tmp;
    int ret;
    disk_t *d;

    if ((d = disk_new(fname, 0)) == NULL)
	return -1;

    ret = 0;
    tmp = NULL;

    if (do_save) {
	tmp = make_needed_name_disk(d);
	if (tmp == NULL) {
	    myerror(ERRDEF, "cannot create needed file name");
	    ret = -1;
	}
	else if (ensure_dir(tmp, 1) < 0)
	    ret = -1;
	else if (rename_or_move(fname, tmp) != 0)
	    ret = -1;
	else {
	    disk_free(d);
	    d = disk_new(tmp, 0);
	}
    }

    ensure_needed_maps();
    enter_disk_in_map(d, FILE_NEEDED);
    disk_free(d);
    free(tmp);
    return ret;
}
