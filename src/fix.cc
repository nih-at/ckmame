/*
  fix.c -- fix ROM sets
  Copyright (C) 1999-2015 Dieter Baron and Thomas Klausner

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

#include <filesystem>
#include <string>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zip.h>

#include "archive.h"
#include "error.h"
#include "file_location.h"
#include "funcs.h"
#include "game.h"
#include "garbage.h"
#include "globals.h"
#include "match.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

static int fix_disks(Game *, Images *, Result *);
static int fix_files(Game *, Archive *, Result *, garbage_t *);
static int fix_files_incomplete(Game *g, Archive *a, Result *res, garbage_t *gb);


int
fix_game(Game *g, Archive *a, Images *im, Result *res) {
    int ret;
    bool move;
    GarbagePtr gb;

    if (fix_options & FIX_DO) {
        gb = std::make_shared<Garbage>(a);

        if (!a->check()) {
	    char *new_name;

	    /* opening the zip file failed, rename it and create new one */

	    if ((new_name = make_unique_name("broken", "%s", a->name.c_str())) == NULL) {
		return -1;
	    }

            if (fix_options & FIX_PRINT) {
		printf("%s: rename broken archive to '%s'\n", a->name.c_str(), new_name);
            }
	    if (rename_or_move(a->name.c_str(), new_name) < 0) {
		free(new_name);
		return -1;
	    }
	    free(new_name);
            if (!a->check()) {
		return -1;
	    }
	}

        if (extra_delete_list) {
	    delete_list_mark(extra_delete_list);
        }
        if (needed_delete_list) {
	    delete_list_mark(needed_delete_list);
        }
        if (superfluous_delete_list) {
	    delete_list_mark(superfluous_delete_list);
        }
    }

    for (uint64_t i = 0; i < a->files.size(); i++) {
        switch (result_file(res, i)) {
        case FS_UNKNOWN:
            if (fix_options & FIX_IGNORE_UNKNOWN) {
                break;
            }
            move = (fix_options & FIX_MOVE_UNKNOWN);
            if (fix_options & FIX_PRINT) {
		printf("%s: %s unknown file '%s'\n", a->name.c_str(), (move ? "move" : "delete"), a->files[i].name.c_str());
            }

	    if (fix_options & FIX_DO) {
                if (move) {
                    gb->add(i, false); /* TODO: check return value */
                }
                else {
		    a->file_delete(i); /* TODO: check return value */
                }
	    }
	    break;

	case FS_DUPLICATE:
            if (!(fix_options & FIX_DELETE_DUPLICATE)) {
		break;
            }
	    /* fallthrough */
	case FS_SUPERFLUOUS:
            if (fix_options & FIX_PRINT) {
		printf("%s: delete %s file '%s'\n", a->name.c_str(), (result_file(res, i) == FS_SUPERFLUOUS ? "unused" : "duplicate"), a->files[i].name.c_str());
            }

	    /* TODO: handle error (how?) */
            a->file_delete(i);
	    break;

	case FS_NEEDED:
	    /* TODO: handle error (how?) */
	    if (save_needed(a, i, g->name.c_str())) {
		tree_recheck_games_needing(check_tree, a->files[i].size, &a->files[i].hashes);
	    }
	    break;

	case FS_BROKEN:
	case FS_USED:
	case FS_MISSING:
	case FS_PARTUSED:
	    /* nothing to be done */
	    break;
	}
    }

    if (fix_options & FIX_DO) {
        if (!gb->commit()) {
	    /* TODO: error message? (or is message from archive_close enough?) */
	    /* TODO: handle error (how?) */
            a->rollback();
	    myerror(ERRZIP, "committing garbage failed");
	    return -1;
	}
    }

    if ((fix_options & FIX_COMPLETE_ONLY) == 0 || res->game == GS_CORRECT || res->game == GS_FIXABLE) {
	ret = fix_files(g, a, res, gb.get());
    }
    else {
	ret = fix_files_incomplete(g, a, res, gb.get());
    }

    if (fix_options & FIX_DO) {
        if (!gb->close()) {
            a->rollback();
	    myerror(ERRZIP, "closing garbage failed");
	    return -1;
	}
    }

    if (!a->commit()) {
        a->rollback();
        if ((fix_options & FIX_DO) && extra_delete_list) {
	    delete_list_rollback(extra_delete_list);
        }
        if ((fix_options & FIX_DO) && needed_delete_list) {
	    delete_list_rollback(needed_delete_list);
        }
        if ((fix_options & FIX_DO) && superfluous_delete_list) {
	    delete_list_rollback(superfluous_delete_list);
        }
    }

    fix_disks(g, im, res);

    return ret;
}


static int
fix_disks(Game *g, Images *im, Result *res) {
    bool do_copy;
    size_t removed = 0;
    bool added = false;

    for (size_t i = 0; i < im->disks.size(); i++) {
	std::string name = images_name(im, i);

	switch (result_image(res, i)) {
	case FS_UNKNOWN:
	case FS_BROKEN:
	    if (fix_options & FIX_PRINT)
		printf("%s: %s unknown image\n", name.c_str(), ((fix_options & FIX_MOVE_UNKNOWN) ? "move" : "delete"));
	    if (fix_options & FIX_DO) {
                if (fix_options & FIX_MOVE_UNKNOWN) {
		    move_image_to_garbage(name);
                }
                else {
		    my_remove(name.c_str());
                }
                removed += 1;
	    }
	    break;

	case FS_DUPLICATE:
	case FS_SUPERFLUOUS:
	    if (fix_options & FIX_PRINT)
		printf("%s: delete %s image\n", name.c_str(), (result_image(res, i) == FS_SUPERFLUOUS ? "unused" : "duplicate"));
            if (fix_options & FIX_DO) {
		my_remove(name.c_str());
                removed += 1;
            }
	    remove_from_superfluous(name.c_str());
	    break;

	case FS_NEEDED:
	    if (fix_options & FIX_PRINT) {
		printf("%s: save needed image\n", name.c_str());
	    }
	    save_needed_disk(name.c_str(), (fix_options & FIX_DO));
            if (fix_options & FIX_DO) {
                removed += 1;
            }
	    break;

	case FS_MISSING:
	case FS_USED:
	case FS_PARTUSED:
	    break;
	}
    }

    for (size_t i = 0; i < g->disks.size(); i++) {
	auto disk = &g->disks[i];
        auto &match_disk = res->disks[i];

	switch (match_disk.quality) {
	case QU_COPIED: {
	    auto name = findfile(disk->name, TYPE_DISK, g->name);
	    if (name != "") {
		myerror(ERRDEF, "internal error: unknown disk '%s' exists, skipping", name.c_str());
		continue;
	    }

	    auto fname = make_file_name(TYPE_DISK, disk->name, g->name);

            switch (match_disk.where) {
            case FILE_INGAME:
            case FILE_NEEDED:
            case FILE_SUPERFLUOUS:
                do_copy = 0;
                break;

            case FILE_INCO:
            case FILE_INGCO:
            case FILE_ROMSET:
                do_copy = 1;
                break;

            case FILE_EXTRA:
                do_copy = (fix_options & FIX_DELETE_EXTRA) == 0;
                break;

	    default:
                /* shouldn't happen */
                do_copy = 1;
                break;
            }

	    if (fix_options & FIX_PRINT)
		printf("%s '%s' to '%s'\n", do_copy ? "copy" : "rename", match_disk.name.c_str(), fname.c_str());
	    if (fix_options & FIX_DO) {
                ensure_dir(fname.c_str(), 1);
		if (do_copy) {
		    link_or_copy(match_disk.name.c_str(), fname.c_str());
#if 0
		    /* delete_list_execute can't currently handle disks */
		    if (extra_delete_list)
		        delete_list_add(extra_delete_list, match_disk_name(md), 0);
#endif
		}
		else {
		    std::error_code ec;
                    auto dir = mydirname(match_disk.name);
                    rename_or_move(match_disk.name.c_str(), fname.c_str());
		    std::filesystem::remove(dir, ec);
		    if (extra_list) {
			int idx;
			idx = parray_find_sorted(extra_list, match_disk.name.c_str(), reinterpret_cast<int (*)(const void *, const void *)>(strcmp));
			if (idx >= 0)
			    parray_delete(extra_list, idx, free);
		    }
		}
                added = true;
	    }
	    remove_from_superfluous(match_disk.name.c_str());

	    break;
	}
	case QU_HASHERR:
	    /* TODO: move to garbage */
	    break;

	default:
	    /* no fix necessary/possible */
	    break;
	}
    }

    if (!added && removed == im->disks.size()) {
	std::error_code ec;
	std::string path = std::string(get_directory()) + "/" + g->name;
	std::filesystem::remove(path, ec);
    }

    return 0;
}


static int
make_space(Archive *a, const char *name, std::vector<std::string> *original_names, size_t num_names) {
    auto idx = a->file_index_by_name(name);

    if (!idx.has_value()) {
	return 0;
    }

    auto index = idx.value();

    if (index < num_names && (*original_names)[index].empty()) {
	(*original_names)[index] = name;
    }

    if (a->files[index].status == STATUS_BADDUMP) {
        if (fix_options & FIX_PRINT) {
	    printf("%s: delete broken '%s'\n", a->name.c_str(), name);
        }
        return a->file_delete(index) ? 0 : -1;
    }

    return a->file_rename_to_unique(index) ? 0 : -1;
}


#define REAL_NAME(aa, ii) ((aa) == a && (ii) < num_names && !original_names[(ii)].empty() ? original_names[(ii)].c_str() : (aa)->files[ii].name.c_str())

static int
fix_files(Game *g, Archive *a, Result *res, garbage_t *gb) {
    Archive *afrom;

    bool needs_recheck = false;

    seterrinfo("", a->name);

    size_t num_names = a->files.size();
    std::vector<std::string> original_names;
    original_names.resize(num_names);

    for (size_t i = 0; i < g->roms.size(); i++) {
        Match *m = result_rom(res, i);

        if (match_source_is_old(m)) {
	    afrom = NULL;
        }
        else {
	    afrom = match_archive(m);
        }
        File *r = &g->roms[i];
	seterrinfo(r->name, a->name);

	switch (match_quality(m)) {
	case QU_MISSING:
	    if (r->size == 0) {
		/* create missing empty file */
		if (fix_options & FIX_PRINT)
		    printf("%s: create empty file '%s'\n", a->name.c_str(), r->name.c_str());

		/* TODO: handle error (how?) */
		a->file_add_empty(r->name.c_str());
	    }
	    break;

	case QU_HASHERR:
	    /* all is lost */
	    break;

        case QU_LONG:
            {
                if (a == afrom && (fix_options & FIX_MOVE_LONG) && afrom->files[match_index(m)].where != FILE_DELETED) {
                    if (fix_options & FIX_PRINT) {
                        printf("%s: move long file '%s'\n", afrom->name.c_str(), REAL_NAME(afrom, match_index(m)));
                    }
                    if (!gb->add(match_index(m), true)) {
                        break;
                    }
                }

                if (fix_options & FIX_PRINT) {
                    printf("%s: extract (offset %jd, size %" PRIu64 ") from '%s' to '%s'\n", a->name.c_str(), (intmax_t)match_offset(m), r->size, REAL_NAME(afrom, match_index(m)), r->name.c_str());
                }

                bool replacing_ourself = (a == afrom && match_index(m) == afrom->file_index_by_name(r->name.c_str()));
                if (make_space(a, r->name.c_str(), &original_names, num_names) < 0) {
                    break;
                }
                if (!a->file_copy_part(afrom, match_index(m), r->name.c_str(), match_offset(m), r->size, r)) {
                    break;
                }
                if (a == afrom && afrom->files[match_index(m)].where != FILE_DELETED) {
                    if (!replacing_ourself && !(fix_options & FIX_MOVE_LONG) && (fix_options & FIX_PRINT)) {
                        printf("%s: delete long file '%s'\n", afrom->name.c_str(), r->name.c_str());
                    }
                    afrom->file_delete(match_index(m));
                }
                break;
            }

	case QU_NAMEERR:
	    if (r->where == FILE_INCO || r->where == FILE_INGCO) {
		if (tree_recheck(check_tree, g->cloneof[r->where - 1].c_str())) {
		    /* fall-through to rename in case save_needed fails */
		    if (save_needed(a, match_index(m), g->name.c_str())) {
			tree_recheck_games_needing(check_tree, r->size, &r->hashes);
			break;
		    }
		}
	    }

	    if (fix_options & FIX_PRINT)
		printf("%s: rename '%s' to '%s'\n", a->name.c_str(), REAL_NAME(a, match_index(m)), r->name.c_str());

	    /* TODO: handle errors (how?) */
	    if (make_space(a, r->name.c_str(), &original_names, num_names) < 0)
		break;
	    a->file_rename(match_index(m), r->name.c_str());

	    break;

	case QU_COPIED:
	    if (gb && afrom == gb->da.get()) {
		/* we can't copy from our own garbage archive, since we're copying to it, and libzip doesn't support cross copying */

		/* TODO: handle error (how?) */
                if (save_needed(afrom, match_index(m), g->name.c_str())) {
		    needs_recheck = true;
                }

		break;
	    }
            if (fix_options & FIX_PRINT) {
		printf("%s: add '%s/%s' as '%s'\n", a->name.c_str(), afrom->name.c_str(), REAL_NAME(afrom, match_index(m)), r->name.c_str());
            }

	    if (make_space(a, r->name.c_str(), &original_names, num_names) < 0) {
		/* TODO: if (idx >= 0) undo deletion of broken file */
		break;
	    }

            if (!a->file_copy(afrom, match_index(m), r->name.c_str())) {
		myerror(ERRDEF, "copying '%s' from '%s' to '%s' failed, not deleting", r->name.c_str(), afrom->name.c_str(), a->name.c_str());
		/* TODO: if (idx >= 0) undo deletion of broken file */
	    }
	    else {
		delete_list_used(afrom, match_index(m));
	    }
	    break;

	case QU_INZIP:
	    /* TODO: save to needed */
	    break;

	case QU_OK:
	    /* all is well */
	    break;

	case QU_NOHASH:
	    /* only used for disks */
	    break;

	case QU_OLD:
	    /* nothing to be done */
	    break;
	}
    }

    return needs_recheck ? 1 : 0;
}


static int
fix_files_incomplete(Game *g, Archive *a, Result *res, garbage_t *gb) {
    Archive *afrom;
    Match *m;

    seterrinfo("", a->name);

    for (size_t i = 0; i < g->roms.size(); i++) {
	m = result_rom(res, i);
	if (match_source_is_old(m))
	    afrom = NULL;
	else
	    afrom = match_archive(m);
	auto r = &g->roms[i];
	seterrinfo(r->name.c_str(), a->name);

	switch (match_quality(m)) {
	case QU_MISSING:
	case QU_OLD:
	    /* nothing to do */
	    break;

	case QU_NOHASH:
	    /* only used for disks */
	    break;

	case QU_HASHERR:
	    /* all is lost */
	    break;

	case QU_LONG:
	    save_needed_part(afrom, match_index(m), g->name.c_str(), match_offset(m), r->size, r); /* TODO: handle error */
	    break;

	case QU_COPIED:
	    switch(match_where(m)) {
	    case FILE_INGAME:
	    case FILE_SUPERFLUOUS:
	    case FILE_EXTRA:
		/* TODO: handle error (how?) */
		save_needed(afrom, match_index(m), g->name.c_str());
		afrom->commit();
		break;

	    case FILE_INCO:
	    case FILE_INGCO:
	    case FILE_ROMSET:
	    case FILE_NEEDED:
	    case FILE_OLD:
            case FILE_NOWHERE:
            case FILE_ADDED:
            case FILE_DELETED:
		/* file is already where we will find it later */
		break;
	    }
	    break;

	case QU_NAMEERR:
	case QU_OK:
	case QU_INZIP:
	    save_needed(a, i, g->name.c_str()); /* TODO: handle error */
	    break;
	}
    }

    return 0;
}
