/*
  delete_list.c -- list of files to delete
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

#include <algorithm>

#include <stdlib.h>
#include <zip.h>

#include "delete_list.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"


int
delete_list_execute(DeleteListPtr dl) {
    std::string name;
    ArchivePtr a = NULL;

    std::sort(dl->entries.begin(), dl->entries.end());

    int ret = 0;
    for (size_t i = 0; i < dl->entries.size(); i++) {
	auto entry = dl->entries[i];

	if (name == "" || entry.name != name) {
	    if (a) {
		if (!a->commit()) {
		    a->rollback();
		    ret = -1;
		}

		if (a->is_empty())
		    remove_empty_archive(name.c_str());

                a = NULL;
	    }

	    name = entry.name;
	    /* TODO: don't hardcode location */
            a = Archive::open(name, TYPE_ROM, FILE_NOWHERE, 0);
            if (!a) {
                ret = -1;
            }
	}
	if (a) {
            if (fix_options & FIX_PRINT) {
		printf("%s: delete used file '%s'\n", name.c_str(), a->files[entry.index].name.c_str());
            }
	    /* TODO: check for error */
	    a->file_delete(entry.index);
	}
    }

    if (a) {
	if (!a->commit()) {
            a->rollback();
	    ret = -1;
	}

	if (a->is_empty()) {
            remove_empty_archive(name.c_str());
	}
    }

    return ret;
}


void
delete_list_mark(DeleteListPtr dl) {
    dl->mark = dl->entries.size();
}


void
delete_list_rollback(DeleteListPtr dl) {
    /* this function should only remove items, so dummy should not be used */
    FileLocation dummy("", 0);
    dl->entries.resize(dl->mark, dummy);
}


void
delete_list_used(Archive *a, size_t index) {
    FileLocation fl(a->name, index);

    switch (a->where) {
    case FILE_NEEDED:
	needed_delete_list->entries.push_back(fl);
	break;
	
    case FILE_SUPERFLUOUS:
	superfluous_delete_list->entries.push_back(fl);
	break;

    case FILE_EXTRA:
	if (fix_options & FIX_DELETE_EXTRA) {
	    extra_delete_list->entries.push_back(fl);
	}
	break;
            
    default:
        break;
    }
}
