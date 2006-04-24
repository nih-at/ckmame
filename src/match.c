/*
  $NiH: match.c,v 1.6 2005/09/27 21:33:02 dillo Exp $

  match.c -- information about ROM/file matches
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "dbl.h"
#include "error.h"
#include "hashes.h"
#include "match.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"



const char *
match_file(match_t *m)
{
    if (match_source_is_old(m))
	return match_old_file(m);
    else
	return rom_name(archive_file(match_archive(m),
				     match_index(m)));
}



void
match_finalize(match_t *m)
{
    if (match_source_is_old(m)) {
	free(match_old_game(m));
	free(match_old_file(m));
    }
    else if (IS_ELSEWHERE(match_where(m)))
	archive_free(match_archive(m));
}



const char *
match_game(match_t *m)
{
    if (match_source_is_old(m))
	return match_old_game(m);
    else
	return archive_name(match_archive(m));
}



void
match_init(match_t *m)
{
    match_quality(m) = QU_MISSING;
    match_where(m) = ROM_NOWHERE;
    match_archive(m) = NULL;
    match_index(m) = -1;
    match_offset(m) = -1;
}
