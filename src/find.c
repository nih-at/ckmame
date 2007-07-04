/*
  $NiH: find.c,v 1.16 2006/10/04 17:36:43 dillo Exp $

  find.c -- find ROM in ROM set or archives
  Copyright (C) 2005-2007 Dieter Baron and Thomas Klausner

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



#include "dbh.h"
#include "file_location.h"
#include "find.h"
#include "funcs.h"
#include "game.h"
#include "globals.h"
#include "hashes.h"
#include "memdb.h"
#include "sq_util.h"
#include "xmalloc.h"



#define QUERY_FILE \
    "select game_id, file_idx, location from file where file_type = ?"
#define QUERY_FILE_HASH_FMT \
    " and (%s = ? or %s is null)"
#define QUERY_FILE_TAIL \
    " order by location"



static find_result_t check_for_file_in_zip(const char *, const rom_t *,
					   match_t *);
static find_result_t check_match_disk_old(const game_t *, const disk_t *,
					  match_disk_t *);
static find_result_t check_match_disk_romset(const game_t *, const disk_t *,
					     match_disk_t *);
static find_result_t check_match_old(const game_t *, const rom_t *, match_t *);
static find_result_t check_match_romset(const game_t *, const rom_t *,
					match_t *);
static find_result_t find_disk_in_db(sqlite3 *, const disk_t *, const char *,
				     match_disk_t *,
				     find_result_t (*)(const game_t *,
						       const disk_t *,
						       match_disk_t *));
static find_result_t find_in_db(sqlite3 *, const rom_t *, const char *, match_t *,
				find_result_t (*)(const game_t *,
						  const rom_t *, match_t *));



find_result_t
find_disk(const disk_t *d, match_disk_t *md)
{
    sqlite3_stmt *stmt;
    char query[1024], *p;
    const char *ht;
    disk_t *dm;
    int i, ret;

    strcpy(query, QUERY_FILE);
    p = query + strlen(query);

    for (i=1; i<=HASHES_TYPE_MAX; i<<=1) {
	if (hashes_has_type(disk_hashes(d), i)) {
	    ht = hash_type_string(i);
	    sprintf(p, QUERY_FILE_HASH_FMT, ht, ht);
	    p += strlen(p);
	}
    }
    strcpy(p, QUERY_FILE_TAIL);
	    
    if (sqlite3_prepare_v2(memdb, query, -1, &stmt, NULL) != SQLITE_OK)
	return FIND_ERROR;

    if (sqlite3_bind_int(stmt, 1, TYPE_DISK) != SQLITE_OK
	|| sq3_set_hashes(stmt, 2, disk_hashes(d), 0) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return FIND_ERROR;
    }

    switch (sqlite3_step(stmt)) {
    case SQLITE_ROW:
	if ((dm=disk_by_id(sqlite3_column_int(stmt, 0))) == NULL) {
	    ret = FIND_ERROR;
	    break;
	}
	if (md) {
	    match_disk_name(md) = xstrdup(disk_name(dm));
	    hashes_copy(match_disk_hashes(md), disk_hashes(dm));
	    match_disk_quality(md) = QU_COPIED;
	    match_disk_where(md) = sqlite3_column_int(stmt, 2);
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

    sqlite3_finalize(stmt);

    return ret;
}



find_result_t find_disk_in_old(const disk_t *d, match_disk_t *md)
{
    if (old_db == NULL)
	return FIND_UNKNOWN;

    return find_disk_in_db(old_db, d, NULL, md, check_match_disk_old);
}



find_result_t
find_disk_in_romset(const disk_t *d, const char *skip, match_disk_t *md)
{
    return find_disk_in_db(db, d, skip, md, check_match_disk_romset);
}



find_result_t
find_in_archives(const rom_t *r, match_t *m)
{
    sqlite3_stmt *stmt;
    char query[1024], *p;
    const char *ht;
    archive_t *a;
    rom_t *f;
    int i, ret, hcol;

    strcpy(query, QUERY_FILE);
    p = query + strlen(query);

    if (rom_size(r) != SIZE_UNKNOWN) {
	strcpy(p, " and size = ?");
	p += strlen(p);
	hcol = 3;
    }
    else
	hcol = 2;
    for (i=1; i<=HASHES_TYPE_MAX; i<<=1) {
	if (hashes_has_type(rom_hashes(r), i)) {
	    ht = hash_type_string(i);
	    sprintf(p, QUERY_FILE_HASH_FMT, ht, ht);
	    p += strlen(p);
	}
    }
    strcpy(p, QUERY_FILE_TAIL);
	    
    if (sqlite3_prepare_v2(memdb, query, -1, &stmt, NULL) != SQLITE_OK)
	return FIND_ERROR;

    if (sqlite3_bind_int(stmt, 1, TYPE_ROM) != SQLITE_OK
	|| sq3_set_hashes(stmt, hcol, rom_hashes(r), 0) != SQLITE_OK) {
	sqlite3_finalize(stmt);
	return FIND_ERROR;
    }
    if (rom_size(r) != SIZE_UNKNOWN)
	if (sqlite3_bind_int(stmt, 2, rom_size(r)) != SQLITE_OK) {
	    sqlite3_finalize(stmt);
	    return FIND_ERROR;
	}

    while ((ret=sqlite3_step(stmt)) == SQLITE_ROW) {
	if ((a=archive_by_id(sqlite3_column_int(stmt, 0))) == NULL) {
	    ret = SQLITE_ERROR;
	    break;
	}
	i = sqlite3_column_int(stmt, 1);
	f = archive_file(a, i);

	if ((hashes_types(rom_hashes(r)) & hashes_types(rom_hashes(f)))
	    != hashes_types(rom_hashes(r))) {
	    archive_file_compute_hashes(a, i,
				hashes_types(rom_hashes(r))|romhashtypes);
	    memdb_update_file(a, i);

	    if (rom_status(f) != STATUS_OK
		|| (archive_file_compare_hashes(a, i, rom_hashes(r))
		    != HASHES_CMP_MATCH)) {
		archive_free(a);
		continue;
	    }
	}
	
	if (m) {
	    match_archive(m) = a;
	    match_index(m) = sqlite3_column_int(stmt, 1);
	    match_where(m) = sqlite3_column_int(stmt, 2);
	    match_quality(m) = QU_COPIED;
	}
	else
	    archive_free(a);

	sqlite3_finalize(stmt);
	return FIND_EXISTS;
    }

    sqlite3_finalize(stmt);

    return ret == SQLITE_DONE ? FIND_UNKNOWN : FIND_ERROR;
}



find_result_t
find_in_old(const rom_t *r, match_t *m)
{
    if (old_db == NULL)
	return FIND_MISSING;
    return find_in_db(old_db, r, NULL, m, check_match_old);
}



find_result_t
find_in_romset(const rom_t *r, const char *skip, match_t *m)
{
    return find_in_db(db, r, skip, m, check_match_romset);
}



static find_result_t
check_for_file_in_zip(const char *name, const rom_t *r, match_t *m)
{
    char *full_name;
    archive_t *a;
    int idx;

    if ((full_name=findfile(name, TYPE_ROM)) == NULL
	|| (a=archive_new(full_name, 0)) == NULL) {
	free(full_name);
	return FIND_MISSING;
    }
    free(full_name);

    if ((idx=archive_file_index_by_name(a, rom_name(r))) >= 0
	&& archive_file_compare_hashes(a, idx,
				       rom_hashes(r)) == HASHES_CMP_MATCH) {
	if (m) {
	    match_archive(m) = a;
	    match_index(m) = idx;
	}
	else
	    archive_free(a);
	return FIND_EXISTS;
    }

    archive_free(a);

    return FIND_MISSING;
}



/*ARGSUSED1*/
static find_result_t
check_match_disk_old(const game_t *g, const disk_t *d, match_disk_t *md)
{
    if (md) {
	match_disk_quality(md) = QU_OLD;
	match_disk_name(md) = xstrdup(disk_name(d));
	hashes_copy(match_disk_hashes(md), disk_hashes(d));
    }
    
    return FIND_EXISTS;
}



/*ARGSUSED1*/
static find_result_t
check_match_disk_romset(const game_t *g, const disk_t *d, match_disk_t *md)
{
    char *file_name;
    disk_t *f;
    
    file_name = findfile(disk_name(d), TYPE_DISK);
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
	    hashes_copy(match_disk_hashes(md),disk_hashes(f));
	}
	else {
	    free(file_name);
	    disk_free(f);
	}
	return FIND_EXISTS;
    }

    free(file_name);
    disk_free(f);
    return FIND_MISSING;
}



static find_result_t
check_match_old(const game_t *g, const rom_t *r, match_t *m)
{
    if (m) {
	match_quality(m) = QU_OLD;
	match_where(m) = ROM_OLD;
	match_old_game(m) = xstrdup(game_name(g));
	match_old_file(m) = xstrdup(rom_name(r));
    }
    
    return FIND_EXISTS;
}



static find_result_t
check_match_romset(const game_t *g, const rom_t *r, match_t *m)
{
    find_result_t status;
    
    status = check_for_file_in_zip(game_name(g), r, m);
    if (m && status == FIND_EXISTS) {
	match_quality(m) = QU_COPIED;
	match_where(m) = ROM_ROMSET;
    }
    
    return status;
}



static find_result_t
find_in_db(sqlite3 *db, const rom_t *r, const char *skip, match_t *m,
	   find_result_t (*check_match)(const game_t *, const rom_t *,
					match_t *))
{
    array_t *a;
    file_location_t *fbh;
    game_t *g;
    const rom_t *gr;
    int i;
    find_result_t status;

    if ((a=r_file_by_hash(db, TYPE_ROM, rom_hashes(r))) == NULL)
	return FIND_UNKNOWN;

    status = FIND_UNKNOWN;
    for (i=0;
	 (status != FIND_ERROR && status != FIND_EXISTS) && i<array_length(a);
	 i++) {
	fbh = array_get(a, i);

	if (skip && strcmp(file_location_name(fbh), skip) == 0)
	    continue;

	if ((g=r_game(db, file_location_name(fbh))) == NULL
	    || game_num_files(g, TYPE_ROM) <= file_location_index(fbh)) {
	    /* XXX: internal error: db inconsistency */
	    status = FIND_ERROR;
	    break;
	}

	gr = game_file(g, TYPE_ROM, file_location_index(fbh));

	if (hashes_cmp(rom_hashes(r), rom_hashes(gr)) == HASHES_CMP_MATCH) {
	    status = check_match(g, gr, m);
	}

	game_free(g);
    }

    array_free(a, file_location_finalize);

    return status;
}



find_result_t
find_disk_in_db(sqlite3 *db, const disk_t *d, const char *skip, match_disk_t *md,
		find_result_t (*check_match)(const game_t *, const disk_t *,
					     match_disk_t *))
{
    array_t *a;
    file_location_t *fbh;
    game_t *g;
    const disk_t *gd;
    int i;
    find_result_t status;

    if ((a=r_file_by_hash(db, TYPE_DISK, disk_hashes(d))) == NULL) {
	/* XXX: internal error: db inconsistency */
	return FIND_ERROR;
    }

    status = FIND_UNKNOWN;
    for (i=0;
	 (status != FIND_ERROR && status != FIND_EXISTS) && i<array_length(a);
	 i++) {
	fbh = array_get(a, i);

	if (skip && strcmp(file_location_name(fbh), skip) == 0)
	    continue;

	if ((g=r_game(db, file_location_name(fbh))) == NULL) {
	    /* XXX: internal error: db inconsistency */
	    status = FIND_ERROR;
	    break;
	}

	gd = game_disk(g, file_location_index(fbh));

	if (hashes_cmp(disk_hashes(d), disk_hashes(gd)) == HASHES_CMP_MATCH)
	    status = check_match(g, gd, md);

	game_free(g);
    }

    array_free(a, file_location_finalize);

    return status;
}
