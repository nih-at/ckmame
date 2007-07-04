/*
  $NiH: diagnostics.c,v 1.13 2007/04/16 16:43:32 dillo Exp $

  diagnostics.c -- display result of check
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

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
#include "pri.h"
#include "types.h"
#include "warn.h"

static const char *zname[] = {
    "",
    "cloneof",
    "grand-cloneof"
};

static void diagnostics_disks(const game_t *, const result_t *);
static void diagnostics_files(const game_t *, const result_t *);



void
diagnostics(const game_t *game, const archive_t *a, const images_t *im,
	    const result_t *res)
{
    warn_set_info(WARN_TYPE_GAME, game_name(game));

    diagnostics_files(game, res);
    diagnostics_archive(a, res);
    diagnostics_disks(game, res);
    diagnostics_images(im, res);
}
    


void
diagnostics_archive(const archive_t *a, const result_t *res)
{
    int i;
    rom_t *f;

    if (a == NULL)
	return;

    for (i=0; i<archive_num_files(a); i++) {
	f = archive_file(a, i);
	
	switch (result_file(res, i)) {
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
	case FS_DUPLICATE:
	    if ((output_options & WARN_SUPERFLUOUS)
		&& (fix_options & FIX_DELETE_DUPLICATE))
		warn_file(f, "duplicate");
	    break;
	}
    }
}



static void
diagnostics_disks(const game_t *game, const result_t *res)
{
    int i;
    disk_t *d;
    match_disk_t *md;

    if (game_num_disks(game) == 0)
	return;

    for (i=0; i<game_num_disks(game); i++) {
	md = result_disk(res, i);
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

	case QU_OLD:
	    if (output_options & WARN_CORRECT)
		warn_disk(d, "old");
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
}



static void
diagnostics_files(const game_t *game, const result_t *res)
{
    match_t *m;
    rom_t *f, *r;
    int i, all_same;
    
    switch (result_game(res)) {
    case GS_CORRECT:
	if (output_options & WARN_CORRECT)
	    warn_rom(NULL, "correct");
	return;
    case GS_OLD:
	if (!(output_options & WARN_CORRECT))
	    return;
	else {
	    all_same = 1;
	    for (i=1; i<game_num_files(game, file_type); i++) {
		if (strcmp(match_old_game(result_rom(res, 0)),
			   match_old_game(result_rom(res, i))) != 0) {
		    all_same = 0;
		    break;
		}
	    }
	    if (all_same) {
		warn_rom(NULL, "old in `%s'",
			 match_old_game(result_rom(res, 0)));
		return;
	    }
	}
	break;
    case GS_MISSING:
	if (output_options & WARN_MISSING)
	    warn_rom(NULL, "not a single rom found");
	return;
    default:
	break;
    }

    for (i=0; i<game_num_files(game, file_type); i++) {
	m = result_rom(res, i);
	r = game_file(game, file_type, i);
	if (!match_source_is_old(m) && match_archive(m))
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
			 "too long, valid subsection at byte %" PRIdoff
			 " (%d)%s%s",
			 PRIoff_cast match_offset(m), rom_size(f),
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
	    
	case QU_OLD:
	    if (output_options & WARN_CORRECT)
		warn_rom(r, "old in `%s'", match_old_game(m));
	    break;
	case QU_NOHASH:
	    /* only used for disks */
	    break;
	}
    }
}



void
diagnostics_images(const images_t *im, const result_t *res)
{
    int i;

    if (im == NULL)
	return;

    for (i=0; i<images_length(im); i++) {
	switch (result_image(res, i)) {
	case FS_UNKNOWN:
	    if (output_options & WARN_UNKNOWN)
		warn_image(images_name(im, i), "unknown");
	    break;
	case FS_BROKEN:
	    if (output_options & WARN_FILE_BROKEN)
		warn_image(images_name(im, i), "broken");
	    break;
	case FS_SUPERFLUOUS:
	    if (output_options & WARN_SUPERFLUOUS)
		warn_image(images_name(im, i), "not used");
	    break;
	case FS_DUPLICATE:
	    if (output_options & WARN_SUPERFLUOUS)
		warn_image(images_name(im, i), "duplicate");
	    break;
	case FS_NEEDED:
	    if (output_options & WARN_USED)
		warn_image(images_name(im, i), "needed elsewhere");
	    break;

	case FS_MISSING:
	case FS_PARTUSED:
	case FS_USED:
	    break;
	}
    }
}
