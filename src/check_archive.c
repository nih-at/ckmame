/*
  check_archive.c -- determine status of files in archive
  Copyright (C) 2005-2006 Dieter Baron and Thomas Klausner

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
check_archive(archive_t *a, const char *gamename, result_t *res)
{
    int i;

    if (a == NULL)
	return;

    for (i=0; i<archive_num_files(a); i++) {
	if (file_status(archive_file(a, i)) != STATUS_OK) {
	    result_file(res, i) = FS_BROKEN;
	    continue;
	}

	if (result_file(res, i) == FS_USED)
	    continue;

	if ((hashes_types(file_hashes(archive_file(a, i))) & romhashtypes)
	    != romhashtypes) {
	    if (archive_file_compute_hashes(a, i, romhashtypes) < 0) {
		result_file(res, i) = FS_BROKEN;
		continue;
	    }
	}

	if (find_in_old(archive_file(a, i), NULL) == FIND_EXISTS) {
	    result_file(res, i) = FS_DUPLICATE;
	    continue;
	}

	switch (find_in_romset(archive_file(a, i), gamename, NULL)) {
	case FIND_UNKNOWN:
	    break;

	case FIND_EXISTS:
	    result_file(res, i) = FS_SUPERFLUOUS;
	    break;

	case FIND_MISSING:
	    if (file_size(archive_file(a, i)) == 0)
		result_file(res, i) = FS_SUPERFLUOUS;
	    else {
		match_t m;
		ensure_needed_maps();
		match_init(&m);
		if (find_in_archives(archive_file(a, i), &m) != FIND_EXISTS)
		    result_file(res, i) = FS_NEEDED;
		else {
		    if (match_where(&m) == FILE_NEEDED)
			result_file(res, i) = FS_SUPERFLUOUS;
		    else
			result_file(res, i) = FS_NEEDED;
		}
		match_finalize(&m);
	    }
	    break;

	case FIND_ERROR:
	    /* XXX: how to handle? */
	    break;
	}
    }
}
