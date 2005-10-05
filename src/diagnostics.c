/*
  $NiH: diagnostics.c,v 1.2 2005/09/27 21:33:02 dillo Exp $

  diagnostics.c -- display result of check
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



#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "archive.h"
#include "funcs.h"
#include "game.h"
#include "globals.h"
#include "hashes.h"
#include "match.h"
#include "match_disk.h"
#include "types.h"
#include "warn.h"

static const char *zname[] = {
    "",
    "cloneof",
    "grand-cloneof"
};

static const char *gname;
static int gnamedone;

static void diagnostics_archive(const archive_t *,
				const file_status_array_t *);
static void diagnostics_disks(const game_t *, const match_disk_array_t *,
			      const file_status_array_t *, const parray_t *);
static void diagnostics_files(const game_t *, const match_array_t *);
static void warn_disk(const disk_t *, const char *, ...);
static void warn_ensure_game(void);
static void warn_file(const rom_t *, const char *, ...);
static void warn_game(const char *);
static void warn_image(const char *, const char *, ...);
static void warn_rom(const rom_t *, const char *, ...);



void
diagnostics(const game_t *game, const archive_t *a, const match_array_t *ma,
	    const match_disk_array_t *mda, const file_status_array_t *fsa,
	    const file_status_array_t *dsa, const parray_t *dn)
{
    warn_game(game_name(game));

    diagnostics_files(game, ma);
    diagnostics_archive(a, fsa);
    diagnostics_disks(game, mda, dsa, dn);
}
    


static void
diagnostics_archive(const archive_t *a, const file_status_array_t *fsa)
{
    int i;
    rom_t *f;

    if (a == NULL)
	return;

    for (i=0; i<archive_num_files(a); i++) {
	f = archive_file(a, i);
	
	switch (file_status_array_get(fsa, i)) {
	case FS_UNKNOWN:
	    if (output_options & WARN_UNKNOWN)
		warn_file(f, "unknown");
	    break;
	    
	case FS_PARTUSED:
	case FS_USED:
	case FS_MISSING:
	    /* handled in diagnostics_files */
	    break;
	    
	case FS_BROKEN:
	    if (output_options & WARN_FILE_BROKEN)
		warn_file(f, "broken");
	    break;

	case FS_NEEDED:
	    if (output_options & WARN_USED)
		warn_file(f, "needed elsewhere");
	    break;
	    
	case FS_SUPERFLUOUS:
	    if (output_options & WARN_SUPERFLUOUS)
		warn_file(f, "not used");
	    break;
	}
    }
}



static void
diagnostics_disks(const game_t *game, const match_disk_array_t *mda,
		  const file_status_array_t *dsa, const parray_t *dn)
{
    int i;
    disk_t *d;
    match_disk_t *md;

    /* XXX: is mda always set if game has disks? */
    if (mda == NULL)
	return;

    for (i=0; i<game_num_disks(game); i++) {
	md = match_disk_array_get(mda, i);
	d = game_disk(game, i);
	    
	switch (match_disk_quality(md)) {
	case QU_MISSING:
	    if ((disk_status(d) != STATUS_NODUMP
		 && (output_options & WARN_MISSING))
		|| (output_options & WARN_NO_GOOD_DUMP))
		warn_disk(d, "missing");
	    break;
		
	case QU_HASHERR:
	    if (output_options & WARN_WRONG_CRC) {
		/* XXX: display checksum(s) */
		warn_disk(d, "wrong checksum");
	    }
	    break;
		
	case QU_NOHASH:
	    if (disk_status(d) == STATUS_NODUMP
		&& (output_options & WARN_NO_GOOD_DUMP))
		warn_disk(d, "exists");
	    else if (output_options & WARN_WRONG_CRC) {
		/* XXX: display checksum(s) */
		warn_disk(d, "no common checksum types");
	    }
	    break;
		
	case QU_OK:
	    if (output_options & WARN_CORRECT)
		warn_disk(d,
			  disk_status(d) == STATUS_OK ? "correct" : "exists");
	    break;

	case QU_COPIED:
	    if (output_options & WARN_ELSEWHERE)
		warn_disk(d, "is at `%s'", match_disk_name(md));
	    break;

	case QU_NAMEERR:
	    if (output_options & WARN_WRONG_NAME)
		warn_disk(d, "wrong name (%s)",
			  match_disk_name(md));
	    break;
		
	default:
	    break;
	}
    }
    
    for (i=0; i<game_num_disks(game); i++) {
	switch (file_status_array_get(dsa, i)) {
	case FS_UNKNOWN:
	    if (output_options & WARN_UNKNOWN)
		warn_image(parray_get(dn, i), "unknown");
	    break;
	case FS_BROKEN:
	    if (output_options & WARN_FILE_BROKEN)
		warn_image(parray_get(dn, i), "broken");
	    break;
	case FS_SUPERFLUOUS:
	    if (output_options & WARN_SUPERFLUOUS)
		warn_image(parray_get(dn, i), "not used");
	    break;
	case FS_NEEDED:
	    if (output_options & WARN_USED)
		warn_image(parray_get(dn, i), "needed elsewhere");
	    break;

	case FS_MISSING:
	case FS_PARTUSED:
	case FS_USED:
	    break;
	}
    }
}



static void
diagnostics_files(const game_t *game, const match_array_t *ma)
{
    int i, alldead, allcorrect, allowndead, hasown;
    match_t *m;
    rom_t *f, *r;
    
    alldead = allcorrect = allowndead = 1;
    hasown = 0;
    for (i=0; i<game_num_files(game, file_type); i++) {
	m = match_array_get(ma, i);
	r = game_file(game, file_type, i);

	if (rom_where(r) == ROM_INZIP) {
	    hasown = 1;
	    
	    if (match_quality(m) != QU_MISSING)
		alldead = allowndead = 0;
	}
	else {
	    if (match_quality(m) != QU_MISSING)
		alldead = 0;
	}
	
	if (match_quality(m) != QU_OK)
	    allcorrect = 0;
    }

    if (((alldead || (hasown && allowndead))
	 && game_num_files(game, file_type) > 0
	 && (output_options & WARN_MISSING))) {
	warn_rom(NULL, "not a single rom found");
    }
    else if (allcorrect && (output_options & WARN_CORRECT)) {
	warn_rom(NULL, "correct");
    }
    else {
        for (i=0; i<game_num_files(game, file_type); i++) {
	    m = match_array_get(ma, i);
	    r = game_file(game, file_type, i);
	    if (match_archive(m))
		f = archive_file(match_archive(m), match_index(m));
	    else
		f = NULL;
	    
	    switch (match_quality(m)) {
	    case QU_MISSING:
		if (output_options & WARN_MISSING) {
		    if (rom_status(r) == STATUS_OK
			|| output_options & WARN_NO_GOOD_DUMP)
			warn_rom(r, "missing");
		}
		break;
		
	    case QU_NAMEERR:
		if (output_options & WARN_WRONG_NAME)
		    warn_rom(r, "wrong name (%s)%s%s",
			     rom_name(f),
			     (rom_where(r) != match_where(m)
			      ? ", should be in " : ""),
			     zname[rom_where(r)]);
		break;
		
	    case QU_LONG:
		if (output_options & WARN_LONGOK)
		    warn_rom(r,
			     "too long, valid subsection at byte %lld (%d)%s%s",
			     match_offset(m), rom_size(f),
			     (rom_where(r) != match_where(m)
			      ? ", should be in " : ""),
			     zname[rom_where(r)]);
		break;
		
	    case QU_OK:
		if (output_options & WARN_CORRECT) {
		    if (rom_status(r) == STATUS_OK)
			warn_rom(r, "correct");
		    else if (output_options & WARN_NO_GOOD_DUMP)
			warn_rom(r,
				 rom_status(r) == STATUS_BADDUMP
				 ? "best bad dump" : "exists");
		}
		break;

	    case QU_COPIED:
		if (output_options & WARN_ELSEWHERE)
		    warn_rom(r, "is in `%s'", archive_name(match_archive(m)));
		break;

	    case QU_INZIP:
		if (output_options & WARN_WRONG_ZIP)
		    warn_rom(r, "should be in %s",
			     zname[rom_where(r)]);
		break;

	    case QU_HASHERR:
		if (output_options & WARN_MISSING) {
		    warn_rom(r, "checksum mismatch%s%s",
			     (rom_where(r) != match_where(m)
			      ? ", should be in " : ""),
			     (rom_where(r) != match_where(m)
			      ? zname[rom_where(r)] : ""));
		}
		break;
		
	    case QU_NOHASH:
		/* only used for disks */
		break;
	    }
	}
    }
}



static void
warn_disk(const disk_t *d, const char *fmt, ...)
{
    va_list va;
    char buf[HASHES_SIZE_MAX*2 + 1];
    const hashes_t *h;

    warn_ensure_game();
    
    printf("disk %-12s  ", disk_name(d));

    h = disk_hashes(d);
    if (hashes_has_type(h, HASHES_TYPE_SHA1))
	printf("sha1 %s: ", hash_to_string(buf, HASHES_TYPE_SHA1, h));
    else if (hashes_has_type(h, HASHES_TYPE_MD5))
	printf("md5 %s         : ", hash_to_string(buf, HASHES_TYPE_MD5, h));
    else
	printf("no good dump              : ");
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}



static void
warn_ensure_game(void)
{
    if (gnamedone == 0) {
	printf("In game %s:\n", gname);
	gnamedone = 1;
    }
}



static void
warn_file(const rom_t *r, const char *fmt, ...)
{
    va_list va;

    warn_ensure_game();
    
    /* XXX */
    printf("file %-12s  size %7ld  crc %.8lx: ",
	   rom_name(r), rom_size(r), hashes_crc(rom_hashes(r)));
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}



static void
warn_game(const char *name)
{
    gname = name;
    gnamedone = 0;
}



static void
warn_image(const char *name, const char *fmt, ...)
{
    va_list va;

    warn_ensure_game();
    
    printf("image %-12s: ", name);
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}



static void
warn_rom(const rom_t *r, const char *fmt, ...)
{
    va_list va;
    char buf[100], *p;
    int j;

    warn_ensure_game();
    
    if (r) {
	printf("rom  %-12s  ", rom_name(r));
	if (rom_size(r) > 0) {
	    sprintf(buf, "size %7ld  ", rom_size(r));
	    p = buf + strlen(buf);
	    
	    /* XXX */
	    if (hashes_has_type(rom_hashes(r), HASHES_TYPE_CRC)) {
		switch (rom_status(r)) {
		case STATUS_OK:
		    sprintf(p, "crc %.8lx: ", hashes_crc(rom_hashes(r)));
		    break;
		case STATUS_BADDUMP:
		    sprintf(p, "bad dump    : ");
		    break;
		case STATUS_NODUMP:
		    sprintf(p, "no good dump: ");
		}
	    } else
		sprintf(p, "no good dump: ");

	}
	else
	    sprintf(p, "                          : ");
	fputs(buf, stdout);
    }
    else
	printf("game %-40s: ", gname);
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    if (r && rom_num_altnames(r)) {
	for (j=0; j<rom_num_altnames(r); j++) {
	    printf("rom  %-12s  ", rom_altname(r, j));
	    fputs(buf, stdout);
	    va_start(va, fmt);
	    vprintf(fmt, va);
	    va_end(va);

	    printf(" (same as %s)\n", rom_name(r));
	}
    }

    return;
}
