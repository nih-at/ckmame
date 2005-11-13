/*
  $NiH: archive.c,v 1.3 2005/10/05 23:00:18 dillo Exp $

  rom.c -- initialize / finalize rom structure
  Copyright (C) 1999, 2004, 2005 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "util.h"
#include "xmalloc.h"

#define BUFSIZE 8192

extern char *prg;

static int get_hashes(struct zip_file *zf, off_t, hashes_t *);
static void read_infos_from_zip(archive_t *, int);



int
archive_close_zip(archive_t *a)
{
    if (archive_zip(a) == NULL)
	return 0;
    
    if (zip_close(archive_zip(a)) < 0) {
	/* error closing, so zip is still valid */
	myerror(ERRZIP, "error closing zip: %s", zip_strerror(archive_zip(a)));

	/* XXX: really do this here? */
	/* discard all changes and close zipfile */
	zip_unchange_all(archive_zip(a));
	zip_close(archive_zip(a));
	archive_zip(a) = NULL;
	return -1;
    }

    archive_zip(a) = NULL;

    return 0;
}



int
archive_ensure_zip(archive_t *a, int createp)
{
    int flags;
    
    if (archive_zip(a))
	return 0;

    flags = ZIP_CHECKCONS;
    if (createp)
	flags |= ZIP_CREATE;

    if ((archive_zip(a)=my_zip_open(archive_name(a), flags)) == NULL)
	return -1;

    return 0;
}



int
archive_file_compare_hashes(archive_t *a, int i, const hashes_t *h)
{
    hashes_t *rh;

    rh = rom_hashes(archive_file(a, i));

    if ((hashes_types(rh) & hashes_types(h)) != hashes_types(h))
	archive_file_compute_hashes(a, i, hashes_types(h)|romhashtypes);

    if (rom_status(archive_file(a, i)) != STATUS_OK)
	return HASHES_CMP_NOCOMMON;

    return hashes_cmp(rh, h);
}



int
archive_file_compute_hashes(archive_t *a, int idx, int hashtypes)
{
    hashes_t h;
    struct zip *za;
    struct zip_file *zf;
    rom_t *r;

    if (archive_ensure_zip(a, 0) != 0)
	return -1;

    za = archive_zip(a);
    r = archive_file(a, idx);

    if ((zf=zip_fopen_index(za, idx, 0)) == NULL) {
	myerror(ERRZIP, "error opening index %d: %s", idx, zip_strerror(za));
	rom_status(r) = STATUS_BADDUMP;
	return -1;
    }

    hashes_types(&h) = hashtypes;
    /* XXX: check return value */
    if (get_hashes(zf, rom_size(r), &h) < 0) {
	zip_fclose(zf);
	rom_status(r) = STATUS_BADDUMP;
	return -1;
    }

    zip_fclose(zf);

    if (hashtypes & HASHES_TYPE_CRC) {
	if (rom_hashes(r)->crc != h.crc) {
	    myerror(ERRZIP, "CRC error in `%s': %lx != %lx",
		    rom_name(archive_file(a, idx)),
		    h.crc, rom_hashes(r)->crc);
	    rom_status(r) = STATUS_BADDUMP;
	    return -1;
	}
    }
    hashes_copy(rom_hashes(r), &h);

    return 0;
}



int
archive_file_find_offset(archive_t *a, int idx, int size, const hashes_t *h)
{
    struct zip_file *zf;
    hashes_t hn;
    unsigned int offset, found;

    hashes_init(&hn);
    hashes_types(&hn) = hashes_types(h);

    if (archive_ensure_zip(a, 0) < 0)
	return -1;

    seterrinfo(zip_get_name(archive_zip(a), idx, 0), archive_name(a));
    if ((zf = zip_fopen_index(archive_zip(a), idx, 0)) == NULL) {
	myerror(ERRZIPFILE, "can't open file: %s",
		zip_strerror(archive_zip(a)));
	return -1;
    }

    found = 0;
    offset = 0;
    while (offset+size <= rom_size(archive_file(a, idx))) {
	if (get_hashes(zf, size, &hn) < 0) {
	    myerror(ERRZIPFILE, "read error: %s",
		    zip_strerror(archive_zip(a)));
	    zip_fclose(zf);
	    return -1;
	}
	
	if (hashes_cmp(h, &hn) == HASHES_CMP_MATCH) {
	    found = 1;
	    break;
	}

	offset += size;
    }

    if (zip_fclose(zf)) {
	myerror(ERRZIPFILE, "close error: %s", zip_strerror(archive_zip(a)));
	return -1;
    }
    
    if (found)
	return offset;
	    
    return -1;
}



int
archive_file_index_by_name(const archive_t *a, const char *name)
{
    int i;

    for (i=0; i<archive_num_files(a); i++) {
	if (compare_names(rom_name(archive_file(a, i)), name) == 0)
	    return i;
    }

    return -1;
}



int
archive_free(archive_t *a)
{
    int ret;

    if (a == NULL)
	return 0;

    ret = archive_close_zip(a);

    free(a->name);
    array_free(archive_files(a), rom_finalize);
    free(a);

    return ret;
}



archive_t *
archive_new(const char *name, filetype_t ft, int createp)
{
    struct archive *a;
    char *full_name;
    int i;
    
    full_name = findfile(name, ft);
    if (full_name == NULL) {
	if (createp)
	    full_name = make_file_name(ft, 0, name);
	else
	    return NULL;
    }
    
    a = xmalloc(sizeof(*a));
    a->name = full_name;
    a->files = array_new(sizeof(rom_t));
    a->za = NULL;
    
    read_infos_from_zip(a, romhashtypes);

    for (i=0; i<archive_num_files(a); i++) {
	/* XXX: rom_state(archive_file(a, i)) = ROM_UNKNOWN; */
	rom_where(archive_file(a, i)) = (where_t) -1;
    }

    return a;
}



int
archive_refresh(archive_t *a)
{
    archive_close_zip(a);
    array_truncate(archive_files(a), 0, rom_finalize);

    read_infos_from_zip(a, romhashtypes);

    return 0;
}



static int
get_hashes(struct zip_file *zf, off_t len, struct hashes *h)
{
    hashes_update_t *hu;
    unsigned char buf[BUFSIZE];
    int n;

    hu = hashes_update_new(h);

    while (len > 0) {
	n = len > sizeof(buf) ? sizeof(buf) : len;

	if (zip_fread(zf, buf, n) != n)
	    return -1;

	hashes_update(hu, buf, n);
	len -= n;
    }

    hashes_update_final(hu);

    return 0;
}



static void
read_infos_from_zip(archive_t *a, int hashtypes)
{
    struct zip *za;
    struct zip_stat zsb;
    rom_t *r;
    int i;
    int zerr;

    zerr = 0;
    if ((za=zip_open(archive_name(a), 0, &zerr)) == NULL) {
	/* no error if file doesn't exist */
	if (zerr != ZIP_ER_OPEN && errno != ENOENT) {
	    char errstr[80];

	    zip_error_to_str(errstr, sizeof(errstr), zerr, errno);
	    myerror(ERRDEF, "error opening zip archive '%s': %s",
		    archive_name(a), errstr);
	}

	return;
    }

    archive_zip(a) = za;

    seterrinfo(NULL, archive_name(a));

    for (i=0; i<zip_get_num_files(za); i++) {
	if (zip_stat_index(za, i, 0, &zsb) == -1) {
	    myerror(ERRZIP, "error stat()ing index %d: %s",
		    i, zip_strerror(za));
	    continue;
	}

	array_grow(archive_files(a), rom_init);
	r = archive_last_file(a);
	rom_size(r) = zsb.size;
	rom_name(r) = xstrdup(zsb.name);
	rom_status(r) = STATUS_OK;

	hashes_init(rom_hashes(r));
	rom_hashes(r)->types = HASHES_TYPE_CRC;
	rom_hashes(r)->crc = zsb.crc;

	if (hashtypes != 0)
	    archive_file_compute_hashes(a, i, hashtypes);
    }
}
