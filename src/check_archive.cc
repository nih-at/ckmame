/*
  check_archive.c -- determine status of files in archive
  Copyright (C) 2005-2015 Dieter Baron and Thomas Klausner

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


#include "archive.h"
#include "find.h"
#include "funcs.h"
#include "globals.h"
#include "result.h"


void
check_archive(ArchivePtr archive, const char *gamename, Result *result) {
    find_result_t found;

    if (!archive) {
	return;
    }

    for (size_t i = 0; i < archive->files.size(); i++) {
        auto &file = archive->files[i];
        
	if (file.status != STATUS_OK) {
	    result->files[i] = FS_BROKEN;
	    continue;
	}

        if (result->files[i] == FS_USED) {
	    continue;
        }

	found = find_in_old(&file, archive.get(), NULL);
	if (found == FIND_EXISTS) {
	    result->files[i] = FS_DUPLICATE;
	    continue;
	}

	found = find_in_romset(&file, archive.get(), gamename, NULL);

	switch (found) {
	case FIND_UNKNOWN:
	    break;

	case FIND_EXISTS:
	    result->files[i] = FS_SUPERFLUOUS;
	    break;

	case FIND_MISSING:
	    if (file.size == 0)
		result->files[i] = FS_SUPERFLUOUS;
	    else if (archive->where == FILE_NEEDED) {
		/* this state does what we want, even if it sounds strange,
		   and saves us from introducing a better one */
		result->files[i] = FS_MISSING;
	    }
	    else {
		Match match;
		ensure_needed_maps();
		if (find_in_archives(&file, &match, false) != FIND_EXISTS)
		    result->files[i] = FS_NEEDED;
		else {
		    if (match.where == FILE_NEEDED)
			result->files[i] = FS_SUPERFLUOUS;
		    else
			result->files[i] = FS_NEEDED;
		}
	    }
	    break;

	case FIND_ERROR:
	    /* TODO: how to handle? */
	    break;
	}
    }
}
