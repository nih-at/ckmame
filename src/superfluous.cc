/*
  superfluous.c -- check for unknown file in rom directories
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Dir.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "romdb.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

static void list_game_directory(parray_t *found, const char *dirname, bool dir_known);

parray_t *
list_directory(const char *dirname, const char *dbname) {
    parray_t *listf, *found;

    listf = NULL;
    
    if (dbname) {
        if ((listf = romdb_read_list(db, DBH_KEY_LIST_GAME)) == NULL) {
            myerror(ERRDEF, "list of games not found in database '%s'", dbname);
            exit(1);
        }
    }

    found = parray_new();

    try {
	 Dir dir(dirname, false);
	 std::filesystem::path filepath;

	 while ((filepath = dir.next()) != "") {
	     if (filepath.filename() == DBH_CACHE_DB_NAME) {
		 continue;
	     }

	     bool known = false;

	     if (std::filesystem::is_directory(filepath)) {
		 if (roms_unzipped) {
		     if (listf) {
			 known = parray_find_sorted(listf, filepath.filename().c_str(), reinterpret_cast<int (*)(const void *, const void *)>(strcmp)) != -1;
		     }
		 }
		 else {
		     bool dir_known = listf ? parray_find_sorted(listf, filepath.filename().c_str(), reinterpret_cast<int (*)(const void *, const void *)>(strcmp)) != -1 : false;
		     list_game_directory(found, filepath.c_str(), dir_known);
		     known = true; /* we don't want directories in superfluous list (I think) */
		 }
	     }
	     else {
		 auto ext = filepath.extension();
		 if (ext != "") {
		     if (!roms_unzipped && ext == ".zip" && listf) {
			 known = parray_find_sorted(listf, filepath.stem().c_str(), reinterpret_cast<int (*)(const void *, const void *)>(strcmp)) != -1;
		     }
		 }
	     }

	     if (!known) {
		 parray_push(found, xstrdup(filepath.c_str()));
	     }
	 }
    }
    catch (...) {
	parray_free(listf, free);
	return found;
    }

    parray_free(listf, free);

    if (parray_length(found) > 0) {
	parray_sort_unique(found, reinterpret_cast<int (*)(const void *, const void *)>(strcmp));
    }

    return found;
}


void
print_superfluous(const parray_t *files) {
    int i;

    if (parray_length(files) == 0)
	return;

    printf("Extra files found:\n");

    for (i = 0; i < parray_length(files); i++)
	printf("%s\n", (char *)parray_get(files, i));
}


static void
list_game_directory(parray_t *found, const char *dirname, bool dir_known) {
    GamePtr game;

    if (dir_known) {
        game = romdb_read_game(db, mybasename(dirname));
    }

    try {
	Dir dir(dirname, false);
	std::filesystem::path filepath;

	while ((filepath = dir.next()) != "") {
	    bool known = false;
	    if (game) {
		if (filepath.extension() == ".chd") {
		    for (size_t i = 0; i < game->disks.size(); i++) {
			if (game->disks[i].name == filepath.stem()) {
			    known = true;
			    break;
			}
		    }
		}
	    }
	    
	    if (!known) {
		parray_push(found, xstrdup(filepath.c_str()));
	    }
	}
    }
    catch (...) {
	return;
    }
}
