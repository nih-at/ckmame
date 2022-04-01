/*
  cleanup.c -- clean up list of zip archives
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

#include "cleanup.h"
#include "globals.h"

#include <algorithm>

#include "check.h"
#include "diagnostics.h"
#include "fix_util.h"
#include "Garbage.h"
#include "warn.h"
#include "CkmameCache.h"


static void cleanup_archive(filetype_t filetype, Archive *archive, Result *result, int flags);


void cleanup_list(const DeleteListPtr& list, int flags, where_t where) {
    list->sort_archives();
    list->sort_entries();
    size_t di = 0;
    auto len = list->entries.size();

    auto warn_needed = !(where == FILE_NEEDED || (where == FILE_EXTRA && configuration.complete_games_only));

    auto n = list->archives.size();
    size_t i = 0;
    while (i < n) {
        auto entry = list->archives[i];
        if (where == FILE_EXTRA && !configuration.extra_directory_move_from_extra(ckmame_cache->get_directory_name_for_archive(entry.name))) {
            i++;
            continue;
        }

	warn_set_info(WARN_TYPE_ARCHIVE, entry.name);
        ArchivePtr a = Archive::open(entry.name, entry.filetype, FILE_NOWHERE, 0);

        if (a) {
            GameArchives archives;
            archives.archive[entry.filetype] = a;
                    
            Result res(nullptr, archives);

            while (di < len) {
                auto fl = list->entries[di];
                /* file lists should know what's toplevel without adding a / to name */
		int cmp;
                if (fl.name[fl.name.length() - 1] == '/' && entry.name[entry.name.length() - 1] != '/') {
                    cmp = entry.name.compare(fl.name.substr(0, fl.name.length() - 1));
                }
                else {
                    cmp = entry.name.compare(fl.name);
                }
                
                if (cmp == 0) {
                    res.archive_files[entry.filetype][fl.index] = FS_USED;
                }
                else if (cmp < 0) {
                    break;
                }
                
                di++;
            }
            
            if (where == FILE_NEEDED) {
                check_needed_files(entry.filetype, a, &res);
            }
            else {
                check_archive_files(entry.filetype, archives, "", &res);
            }
            
	    diagnostics_archive(entry.filetype, a.get(), res, warn_needed);
            cleanup_archive(entry.filetype, a.get(), &res, flags);
	    warn_unset_info();
	}

	if (n != list->archives.size()) {
	    n = list->archives.size();
	}
	else {
	    i++;
	}
    }
}


static void cleanup_archive(filetype_t filetype, Archive *a, Result *result, int flags) {
    GarbagePtr gb;

    if (!a->is_writable()) {
        return;
    }
    
    if ((flags & CLEANUP_UNKNOWN) && configuration.fix_romset) {
        gb = std::make_shared<Garbage>(a);
    }

    for (size_t i = 0; i < a->files.size(); i++) {
	switch (result->archive_files[filetype][i]) {
	case FS_SUPERFLUOUS:
	case FS_DUPLICATE:
	case FS_USED: {
	    const char *reason;
	    switch (result->archive_files[filetype][i]) {
	    case FS_SUPERFLUOUS:
		reason = "unused";
		break;
	    case FS_DUPLICATE:
		reason = "duplicate";
		break;
	    case FS_USED:
		reason = "used";
		break;
	    default:
		reason = "[internal error]";
		break;
	    }

	    output.message_verbose("delete %s file '%s'", reason, a->files[i].filename().c_str());
	    a->file_delete(i);
	    break;

	}
	case FS_BROKEN:
	case FS_MISSING:
	case FS_PARTUSED:
	    break;

	case FS_NEEDED:
	    if (flags & CLEANUP_NEEDED) {
		/* TODO: handle error (how?) */
		if (save_needed(a, i, "")) {
		    /* save_needed delays deletes in archives with where != FILE_ROM */
		    a->file_delete(i);
		}
	    }
	    break;

	case FS_UNKNOWN:
	    if (flags & CLEANUP_UNKNOWN) {
                if (a->files[i].hashes.size == 0) {
                    output.message_verbose("delete empty file '%s'", a->files[i].filename().c_str());
                    a->file_delete(i);
                }
                else {
                    output.message_verbose("move unknown file '%s'", a->files[i].filename().c_str());

                    /* TODO: handle error (how?) */
                    if (configuration.fix_romset) {
                        gb->add(i, false);
                    }
                    else {
                        /* when FIX_DO is not set, this only updates in-memory representation of a */
                        a->file_delete(i);
                    }
                }
            }
	    break;
        }
    }

    if (gb && !gb->close()) {
        a->rollback();
    }

    a->commit();

    if (a->is_empty()) {
        remove_empty_archive(a);
    }
}
