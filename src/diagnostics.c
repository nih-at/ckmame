/*
  $NiH$

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
#include "hashes.h"
#include "match.h"
#include "match_disk.h"
#include "types.h"
#include "warn.h"

static const char *zname[] = {
    "zip",
    "cloneof",
    "grand-cloneof"
};

static const char *gname;
static int gnamedone;

static void warn_disk(const disk_t *d, const char *fmt, ...);
static void warn_ensure_game(void);
static void warn_file(const rom_t *r, const char *fmt, ...);
static void warn_game(const char *name);
static void warn_rom(const rom_t *rom, const char *fmt, ...);



void
diagnostics(const game_t *game, filetype_t ft,
	    const match_array_t *ma, const match_disk_array_t *mda,
	    const archive_t **zip)
{
    int i, alldead, allcorrect, allowndead, hasown;
    match_t *m;
    match_disk_t *md;
    rom_t *r, *f;
    disk_t *d;
    
    warn_game(game_name(game));

    /* analyze result: ROMs */
    
    alldead = allcorrect = allowndead = 1;
    hasown = 0;
    for (i=0; i<game_num_files(game, ft); i++) {
	m = match_array_get(ma, i, 0);
	r = game_file(game, ft, i);

	if (rom_where(r) == ROM_INZIP) {
	    hasown = 1;
	    
	    if (match_quality(m) >= ROM_NAMERR)
		alldead = allowndead = 0;
	}
	else {
	    if (match_quality(m) >= ROM_NAMERR)
		alldead = 0;
	}
	
	if (match_where(m) != rom_where(r) || match_quality(m) != ROM_OK)
	    allcorrect = 0;
    }

    if (((alldead || (hasown && allowndead)) && game_num_files(game, ft) > 0
	 && (output_options & WARN_MISSING))) {
	warn_rom(NULL, "not a single rom found");
    }
    else if (allcorrect && (output_options & WARN_CORRECT)) {
	warn_rom(NULL, "correct");
    }
    else {
        for (i=0; i<game_num_files(game, ft); i++) {
	    m = match_array_get(ma, i, 0);
	    r = game_file(game, ft, i);
	    if (match_zno(m) >= 0 && match_fno(m) >= 0)
		f = archive_file(zip[match_zno(m)], match_fno(m));
	    else
		f = NULL;
	    
	    switch (match_quality(m)) {
	    case ROM_UNKNOWN:
		if (output_options & WARN_MISSING) {
		    /* XXX */
		    if (((hashes_crc(rom_hashes(r)) != 0
			  && rom_flags(r) != FLAGS_NODUMP)
			 || rom_size(r) == 0)
			|| output_options & WARN_NO_GOOD_DUMP)
			warn_rom(r, "missing");
		}
		break;
		
	    case ROM_SHORT:
		if (output_options & WARN_SHORT)
		    warn_rom(r, "short (%d)", rom_size(f));
		break;
		
	    case ROM_LONG:
		if (output_options & WARN_LONG)
		    warn_rom(r, "too long, unfixable (%d)", rom_size(f));
		break;
		
	    case ROM_CRCERR:
		if (output_options & WARN_WRONG_CRC)
		    warn_rom(r, "wrong crc (%0.8x)",
			     hashes_crc(rom_hashes(f)));
		break;

	    case ROM_NAMERR:
		if (output_options & WARN_WRONG_NAME)
		    warn_rom(r, "wrong name (%s)", rom_name(f));
		break;
		
	    case ROM_LONGOK:
		if (output_options & WARN_LONGOK)
		    warn_rom(r,
			     "too long, valid subsection at byte %d (%d)",
			     (int)match_offset(m), rom_size(f));
		break;
		
	    case ROM_BESTBADDUMP:
		if (output_options & (WARN_NO_GOOD_DUMP|WARN_CORRECT))
		    warn_rom(r, "best bad dump");
		break;
		
	    case ROM_OK:
		/* XXX */
		if ((hashes_crc(rom_hashes(r)) == 0 || rom_flags(r)
		     == FLAGS_NODUMP) && rom_size(r) > 0
		    && (output_options & WARN_NO_GOOD_DUMP))
		    warn_rom(r, "exists");
		else if (rom_where(r) == match_zno(m) &&
			 (output_options & WARN_CORRECT))
		    warn_rom(r, "correct");
		break;
		
	    default:
		break;
	    }
	    

	    if (match_quality(m) != ROM_UNKNOWN
		&& rom_where(r) != match_zno(m)
		&& (output_options & WARN_WRONG_ZIP))
		warn_rom(r, "should be in %s, is in %s",
			 zname[rom_where(r)],
			 zname[match_zno(m)]);
	}
    }

    
    /* analyze result: files */

    if (zip[0]) {
	for (i=0; i<archive_num_files(zip[0]); i++) {
	    f = archive_file(zip[0], i);
	    
	    if ((rom_state(f) == ROM_UNKNOWN
		 || (rom_state(f) < ROM_NAMERR
		     && rom_where(f) != 0))
		&& (output_options & WARN_UNKNOWN))
		warn_file(f, "unknown");
	    else if (rom_state(f) < ROM_TAKEN
		     && (output_options & WARN_NOT_USED))
		warn_file(f, "not used");
	    else if (rom_where(f) != 0
		     && (output_options & WARN_USED))
		warn_file(f, "used in clone %s",
			  game_clone(game, ft, rom_where(f)-1));
	}
    }

    /* analyze result: disks */

    /* XXX: is mda always set if game has disks? */
    if (mda) {
	for (i=0; i<game_num_disks(game); i++) {
	    md = match_disk_array_get(mda, i);
	    d = game_disk(game, i);
	    
	    switch (match_disk_quality(md)) {
	    case ROM_UNKNOWN:
		if ((disk_flags(d) != FLAGS_NODUMP
		     && (output_options & WARN_MISSING))
		    || (output_options & WARN_NO_GOOD_DUMP))
		    warn_disk(d, "missing");
		break;
		
	    case ROM_CRCERR:
		if (output_options & WARN_WRONG_CRC) {
		    /* XXX: display checksum(s) */
		    warn_disk(d, "wrong checksum");
		}
		break;
		
	    case ROM_NOCRC:
		if (disk_flags(d) == FLAGS_NODUMP
		    && (output_options & WARN_NO_GOOD_DUMP))
		    warn_disk(d, "exists");
		else if (output_options & WARN_WRONG_CRC) {
		    /* XXX: display checksum(s) */
		    warn_disk(d, "no common checksum types");
		}
		break;
		
		
	    case ROM_OK:
		if (output_options & WARN_CORRECT)
		    warn_disk(d, "correct");
		break;
		
	    default:
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
		switch (rom_flags(r)) {
		case FLAGS_OK:
		    sprintf(p, "crc %.8lx: ", hashes_crc(rom_hashes(r)));
		    break;
		case FLAGS_BADDUMP:
		    sprintf(p, "bad dump    : ");
		    break;
		case FLAGS_NODUMP:
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
