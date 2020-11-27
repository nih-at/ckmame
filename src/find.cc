/*
  find.c -- find ROM in ROM set or archives
  Copyright (C) 2005-2014 Dieter Baron and Thomas Klausner

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

#include "find.h"
#include "dbh.h"
#include "file_location.h"
#include "funcs.h"
#include "game.h"
#include "globals.h"
#include "hashes.h"
#include "memdb.h"
#include "sq_util.h"
#include "xmalloc.h"

#define QUERY_FILE "select game_id, file_idx, file_sh, location from file where file_type = ?"
#define QUERY_FILE_HASH_FMT " and (%s = ? or %s is null)"
#define QUERY_FILE_TAIL " order by location"
#define QUERY_FILE_GAME_ID 0
#define QUERY_FILE_FILE_IDX 1
#define QUERY_FILE_FILE_SH 2
#define QUERY_FILE_LOCATION 3


static find_result_t check_for_file_in_zip(const char *, const File *, const File *, Match *);
static find_result_t check_match_disk_old(const Game *, const disk_t *, match_disk_t *);
static find_result_t check_match_disk_romset(const Game *, const disk_t *, match_disk_t *);
static find_result_t check_match_old(const Game *, const File *, const File *, Match *);
static find_result_t check_match_romset(const Game *, const File *, const File *, Match *);
static find_result_t find_disk_in_db(romdb_t *, const disk_t *, const char *, match_disk_t *, find_result_t (*)(const Game *, const disk_t *, match_disk_t *));
static find_result_t find_in_db(romdb_t *, const File *, Archive *, const char *, Match *, find_result_t (*)(const Game *, const File *, const File *, Match *));


find_result_t
find_disk(const disk_t *d, match_disk_t *md) {
    sqlite3_stmt *stmt;
    disk_t *dm;
    int ret;
    
    if (memdb == NULL) {
        return FIND_UNKNOWN;
    }

    if ((stmt = dbh_get_statement(memdb, dbh_stmt_with_hashes_and_size(DBH_STMT_MEM_QUERY_FILE, disk_hashes(d), 0))) == NULL)
	return FIND_ERROR;

    if (sqlite3_bind_int(stmt, 1, TYPE_DISK) != SQLITE_OK || sq3_set_hashes(stmt, 2, disk_hashes(d), 0) != SQLITE_OK) {
	return FIND_ERROR;
    }

    switch (sqlite3_step(stmt)) {
    case SQLITE_ROW:
	if ((dm = disk_by_id(sqlite3_column_int(stmt, QUERY_FILE_GAME_ID))) == NULL) {
	    ret = FIND_ERROR;
	    break;
	}
	if (md) {
	    match_disk_name(md) = xstrdup(disk_name(dm));
	    hashes_copy(match_disk_hashes(md), disk_hashes(dm));
	    match_disk_quality(md) = QU_COPIED;
	    match_disk_where(md) = static_cast<where_t>(sqlite3_column_int(stmt, QUERY_FILE_LOCATION));
	}
	disk_free(dm);
	ret = FIND_EXISTS;
	break;

    case SQLITE_DONE:
	ret = FIND_UNKNOWN;
	break;

    default:
	ret = FIND_ERROR;
	break;
    }

    return static_cast<find_result_t>(ret);
}


find_result_t
find_disk_in_old(const disk_t *d, match_disk_t *md) {
    if (old_db == NULL)
	return FIND_UNKNOWN;

    return find_disk_in_db(old_db, d, NULL, md, check_match_disk_old);
}


find_result_t
find_disk_in_romset(const disk_t *d, const char *skip, match_disk_t *md) {
    return find_disk_in_db(db, d, skip, md, check_match_disk_romset);
}


find_result_t
find_in_archives(const File *r, Match *m, bool needed_only) {
    sqlite3_stmt *stmt;
    File *f;
    int i, ret, hcol, sh;

    if (memdb_ensure() < 0) {
        return FIND_ERROR;
    }
    
    if ((stmt = dbh_get_statement(memdb, dbh_stmt_with_hashes_and_size(DBH_STMT_MEM_QUERY_FILE, file_hashes(r), file_size_(r) != SIZE_UNKNOWN))) == NULL)
	return FIND_ERROR;

    hcol = 2;
    if (file_size_(r) != SIZE_UNKNOWN) {
	hcol++;
	if (sqlite3_bind_int64(stmt, 2, file_size_(r)) != SQLITE_OK)
	    return FIND_ERROR;
    }

    if (sqlite3_bind_int(stmt, 1, TYPE_ROM) != SQLITE_OK || sq3_set_hashes(stmt, hcol, file_hashes(r), 0) != SQLITE_OK)
	return FIND_ERROR;


    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        auto a = Archive::by_id(sqlite3_column_int(stmt, QUERY_FILE_GAME_ID));
        if (!a) {
	    ret = SQLITE_ERROR;
	    break;
        }
	if (needed_only && a->where != FILE_NEEDED) {
	    continue;
	}

	i = sqlite3_column_int(stmt, QUERY_FILE_FILE_IDX);
	sh = sqlite3_column_int(stmt, QUERY_FILE_FILE_SH);
        f = &a->files[i];

	if (sh == FILE_SH_FULL && ((hashes_types(file_hashes(r)) & hashes_types(file_hashes(f))) != hashes_types(file_hashes(r)))) {
	    a->file_compute_hashes(i, hashes_types(file_hashes(r)) | romdb_hashtypes(db, TYPE_ROM));
	    memdb_update_file(a.get(), i);
	}

	if (file_status_(f) != STATUS_OK || (hashes_cmp(file_hashes_xxx(f, sh), file_hashes(r)) != HASHES_CMP_MATCH)) {
	    continue;
	}

	if (m) {
            m->archive = a;
            m->index = i;
            m->where = static_cast<where_t>(sqlite3_column_int(stmt, QUERY_FILE_LOCATION));
	    m->quality = QU_COPIED;
	}

	return FIND_EXISTS;
    }

    return ret == SQLITE_DONE ? FIND_UNKNOWN : FIND_ERROR;
}


find_result_t
find_in_old(const File *r, Archive *a, Match *m) {
    if (old_db == NULL) {
	return FIND_MISSING;
    }

    return find_in_db(old_db, r, a, NULL, m, check_match_old);
}


find_result_t
find_in_romset(const File *r, Archive *a, const char *skip, Match *m) {
    return find_in_db(db, r, a, skip, m, check_match_romset);
}


static find_result_t
check_for_file_in_zip(const char *name, const File *r, const File *f, Match *m) {
    char *full_name;
    ArchivePtr a;

    if ((full_name = findfile(name, TYPE_ROM, NULL)) == NULL || !(a = Archive::open(full_name, TYPE_ROM, FILE_ROMSET, 0))) {
	free(full_name);
	return FIND_MISSING;
    }
    free(full_name);

    auto idx = a->file_index_by_name(file_name(r));
    
    if (idx.has_value() && a->file_compare_hashes(idx.value(), file_hashes(f)) == HASHES_CMP_MATCH) {
        auto index = idx.value();
        File *af = &a->files[index];
	if ((hashes_types(file_hashes(af)) & hashes_types(file_hashes(f))) != hashes_types(file_hashes(f))) {
	    if (!a->file_compute_hashes(index, HASHES_TYPE_ALL)) { /* TODO: only needed hash types */
		return FIND_MISSING;
	    }
	    if (a->file_compare_hashes(index, file_hashes(f)) != HASHES_CMP_MATCH) {
		return FIND_MISSING;
	    }
	}
	if (m) {
            m->archive = a;
            m->index = index;
	}
	return FIND_EXISTS;
    }

    return FIND_MISSING;
}


/*ARGSUSED1*/
static find_result_t
check_match_disk_old(const Game *g, const disk_t *d, match_disk_t *md) {
    if (md) {
	match_disk_quality(md) = QU_OLD;
	match_disk_name(md) = xstrdup(disk_name(d));
	hashes_copy(match_disk_hashes(md), disk_hashes(d));
    }

    return FIND_EXISTS;
}


/*ARGSUSED1*/
static find_result_t
check_match_disk_romset(const Game *g, const disk_t *d, match_disk_t *md) {
    char *file_name;
    disk_t *f;

    file_name = findfile(disk_name(d), TYPE_DISK, game_name(g));
    if (file_name == NULL)
	return FIND_MISSING;

    f = disk_new(file_name, DISK_FL_QUIET);
    if (!f) {
	free(file_name);
	return FIND_MISSING;
    }

    if (hashes_cmp(disk_hashes(d), disk_hashes(f)) == HASHES_CMP_MATCH) {
	if (md) {
	    match_disk_quality(md) = QU_COPIED;
	    match_disk_name(md) = file_name;
	    hashes_copy(match_disk_hashes(md), disk_hashes(f));
	}
	else {
	    free(file_name);
	}
        disk_free(f);
	return FIND_EXISTS;
    }

    free(file_name);
    disk_free(f);
    return FIND_MISSING;
}


static find_result_t
check_match_old(const Game *g, const File *r, const File *f, Match *m) {
    if (m) {
	match_quality(m) = QU_OLD;
	match_where(m) = FILE_OLD;
	match_old_game(m) = game_name(g);
	match_old_file(m) = file_name(r);
    }

    return FIND_EXISTS;
}


static find_result_t
check_match_romset(const Game *g, const File *r, const File *f, Match *m) {
    find_result_t status;

    status = check_for_file_in_zip(game_name(g), r, f, m);
    if (m && status == FIND_EXISTS) {
	match_quality(m) = QU_COPIED;
	match_where(m) = FILE_ROMSET;
    }

    return status;
}


static find_result_t
find_in_db(romdb_t *fdb, const File *r, Archive *archive, const char *skip, Match *m, find_result_t (*check_match)(const Game *, const File *, const File *, Match *)) {
    array_t *a;
    file_location_t *fbh;
    Game *g;
    const File *gr;
    int i;
    find_result_t status;

    if ((a = romdb_read_file_by_hash(fdb, TYPE_ROM, file_hashes(r))) == NULL)
	return FIND_UNKNOWN;

    status = FIND_UNKNOWN;
    for (i = 0; (status != FIND_ERROR && status != FIND_EXISTS) && i < array_length(a); i++) {
	fbh = static_cast<file_location_t *>(array_get(a, i));

	if (skip && strcmp(file_location_name(fbh), skip) == 0)
	    continue;

	if ((g = romdb_read_game(fdb, file_location_name(fbh))) == NULL || game_num_roms(g) <= file_location_index(fbh)) {
	    /* TODO: internal error: database inconsistency */
	    game_free(g);
	    status = FIND_ERROR;
	    break;
	}

	gr = game_rom(g, file_location_index(fbh));

	if (file_size_(r) == file_size_(gr) && hashes_cmp(file_hashes(r), file_hashes(gr)) == HASHES_CMP_MATCH) {
	    bool ok = true;

	    if (archive && (hashes_types(file_hashes(gr)) & (hashes_types(file_hashes(r)))) != hashes_types(file_hashes(gr))) {
		auto idx = archive->file_index(r);
                if (idx.has_value()) {
		    if (!archive->file_compute_hashes(idx.value(), hashes_types(file_hashes(gr)))) {
			/* TODO: handle error (how?) */
			ok = false;
		    }
		}

		if (hashes_cmp(file_hashes(gr), file_hashes(r)) != HASHES_CMP_MATCH) {
		    ok = false;
		}
	    }

	    if (ok) {
		status = check_match(g, gr, r, m);
	    }
	}

	game_free(g);
    }

    array_free(a, reinterpret_cast<void (*)(void *)>(file_location_finalize));

    return status;
}


find_result_t
find_disk_in_db(romdb_t *fdb, const disk_t *d, const char *skip, match_disk_t *md, find_result_t (*check_match)(const Game *, const disk_t *, match_disk_t *)) {
    array_t *a;
    file_location_t *fbh;
    Game *g;
    const disk_t *gd;
    int i;
    find_result_t status;

    if ((a = romdb_read_file_by_hash(fdb, TYPE_DISK, disk_hashes(d))) == NULL) {
	/* TODO: internal error: database inconsistency */
	return FIND_ERROR;
    }

    status = FIND_UNKNOWN;
    for (i = 0; (status != FIND_ERROR && status != FIND_EXISTS) && i < array_length(a); i++) {
	fbh = static_cast<file_location_t *>(array_get(a, i));

	if (skip && strcmp(file_location_name(fbh), skip) == 0)
	    continue;

	if ((g = romdb_read_game(fdb, file_location_name(fbh))) == NULL) {
	    /* TODO: internal error: db inconsistency */
	    status = FIND_ERROR;
	    break;
	}

	gd = game_disk(g, file_location_index(fbh));

	if (hashes_cmp(disk_hashes(d), disk_hashes(gd)) == HASHES_CMP_MATCH)
	    status = check_match(g, gd, md);

	game_free(g);
    }

    array_free(a, reinterpret_cast<void (*)(void *)>(file_location_finalize));

    return status;
}
