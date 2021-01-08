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


#include "funcs.h"
#include "garbage.h"
#include "globals.h"
#include "util.h"
#include "warn.h"

static void cleanup_archive(Archive *, Result *, int);
static void cleanup_disk(Images *, Result *, int);


void
cleanup_list(parray_t *list, delete_list_t *del, int flags) {
    ArchivePtr a;
    char *name;
    int i, di, len, cmp, n;
    file_location_t *fl;
    name_type_t nt;

    di = len = 0;
    if (del) {
	delete_list_sort(del);
	len = delete_list_length(del);
    }

    n = parray_length(list);
    i = 0;
    while (i < n) {
	name = (char *)parray_get(list, i);
	switch ((nt = name_type(name))) {
            case NAME_ZIP: {
                ArchivePtr a = Archive::open(name, TYPE_ROM, FILE_NOWHERE, 0);

                if (!a) {
                    /* TODO */
                    continue;
                }

                Result res(NULL, a.get(), NULL);

                while (di < len) {
                    fl = delete_list_get(del, di);
                    cmp = strcmp(name, file_location_name(fl));

                    if (cmp == 0) {
                        res.files[file_location_index(fl)] = FS_USED;
                    }
                    else if (cmp < 0) {
                        break;
                    }

                    di++;
                }

                check_archive(a, NULL, &res);

                warn_set_info(WARN_TYPE_ARCHIVE, a->name.c_str());
                diagnostics_archive(a, &res);

                cleanup_archive(a.get(), &res, flags);

                break;
            }

            case NAME_CHD: {
                ImagesPtr im = Images::from_file(name);

                if (im == NULL) {
                    /* TODO */
                    continue;
                }

                Result res(NULL, NULL, im.get());

                while (di < len) {
                    fl = delete_list_get(del, di++);
                    cmp = strcmp(name, file_location_name(fl));

                    if (cmp == 0) {
                        res.images[0] = FS_USED;
                    }
                    else if (cmp < 0) {
                        break;
                    }
                }

                check_images(im.get(), NULL, &res);

                warn_set_info(WARN_TYPE_IMAGE, name);
                diagnostics_images(im.get(), &res);

                cleanup_disk(im.get(), &res, flags);

                break;
            }

            case NAME_UNKNOWN:
                /* unknown files shouldn't be in list */
                break;
        }

        if (n != parray_length(list))
            n = parray_length(list);
	else
	    i++;
    }
}


static void
cleanup_archive(Archive *a, Result *res, int flags) {
    GarbagePtr gb;
    int move;

    if ((flags & CLEANUP_UNKNOWN) && (fix_options & FIX_DO)) {
        gb = std::make_shared<Garbage>(a);
    }

    for (size_t i = 0; i < a->files.size(); i++) {
	switch (result_file(res, i)) {
	case FS_SUPERFLUOUS:
	case FS_DUPLICATE:
	case FS_USED: {
	    const char *reason;
	    switch (result_file(res, i)) {
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

	    if (fix_options & FIX_PRINT)
		printf("%s: delete %s file '%s'\n", a->name.c_str(), reason, a->files[i].name.c_str());
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
		if (save_needed(a, i, NULL)) {
		    /* save_needed delays deletes in archives with where != FILE_ROM */
		    a->file_delete(i);
		}
	    }
	    break;

	case FS_UNKNOWN:
	    if (flags & CLEANUP_UNKNOWN) {
		move = fix_options & FIX_MOVE_UNKNOWN;
		if (fix_options & FIX_PRINT) {
		    printf("%s: %s unknown file '%s'\n", a->name.c_str(), (move ? "move" : "delete"), a->files[i].name.c_str());
		}

		/* TODO: handle error (how?) */
		if (move) {
		    if (fix_options & FIX_DO) {
			gb->add(i, false);
		    }
		    else {
			/* when FIX_DO is not set, this only updates in-memory representation of a */
			a->file_delete(i);
		    }
		}
		else {
		    a->file_delete(i);
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
        remove_empty_archive(a->name.c_str());
    }
}


static void
cleanup_disk(Images *im, Result *res, int flags) {
    for (size_t i = 0; i < im->disks.size(); i++) {
	std::string name = images_name(im, i);
	if (name == "") {
	    continue;
	}

	switch (result_image(res, i)) {
	case FS_SUPERFLUOUS:
	case FS_DUPLICATE:
	case FS_USED: {
	    const char *reason;
	    switch (result_image(res, i)) {
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

	    if (fix_options & FIX_PRINT) {
		printf("%s: delete %s image\n", name.c_str(), reason);
	    }
	    if (fix_options & FIX_DO) {
		if (my_remove(name.c_str()) == 0) {
		    remove_from_superfluous(name.c_str());
		}
	    }
	    break;
	}
	case FS_BROKEN:
	case FS_MISSING:
	case FS_PARTUSED:
	    break;

	case FS_NEEDED:
	    if (flags & CLEANUP_NEEDED) {
		if (fix_options & FIX_PRINT) {
		    printf("%s: save needed image\n", name.c_str());
		}
		save_needed_disk(name.c_str(), (fix_options & FIX_DO));
		if (fix_options & FIX_DO) {
		    remove_from_superfluous(name.c_str());
		}
	    }
	    break;

	case FS_UNKNOWN:
	    if (flags & CLEANUP_UNKNOWN) {
		int move, ret;

		move = fix_options & FIX_MOVE_UNKNOWN;
		if (fix_options & FIX_PRINT) {
		    printf("%s: %s unknown image\n", name.c_str(), (move ? "move" : "delete"));
		}
		if (fix_options & FIX_DO) {
		    if (move) {
			ret = move_image_to_garbage(name);
		    }
		    else {
			ret = my_remove(name.c_str());
		    }
		    if (ret == 0) {
			remove_from_superfluous(name.c_str());
		    }
		}
	    }
	    break;
	}
    }
}
