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

#include "DeleteList.h"

#include <algorithm>

#include "fix_util.h"
#include "fix.h"
#include "globals.h"
#include "util.h"


DeleteListPtr extra_delete_list;
DeleteListPtr needed_delete_list;
DeleteListPtr superfluous_delete_list;


DeleteList::Mark::Mark(DeleteListPtr list_) : list(list_), index(0), rollback(false) {
    if (list_) {
        index = list_->entries.size();
        rollback = true;
    }
}


DeleteList::Mark::~Mark() {
    auto l = list.lock();
    
    if (rollback && l && l->entries.size() > index) {
        l->entries.resize(index);
    }
}


int DeleteList::execute() {
    std::string name;
    ArchivePtr a = NULL;

    std::sort(entries.begin(), entries.end());

    int ret = 0;
    for (size_t i = 0; i < entries.size(); i++) {
	auto entry = entries[i];

	if (name == "" || entry.name != name) {
            if (!close_archive(a.get())) {
                ret = -1;
            }
            a = NULL;

	    name = entry.name;
            
            if (name[name.length() - 1] == '/') {
                a = Archive::open(name, roms_unzipped ? TYPE_ROM : TYPE_DISK, FILE_NOWHERE, 0);
            }
            else {
                filetype_t filetype;
                switch (name_type(name)) {
                    case NAME_ZIP:
                        filetype = TYPE_ROM;
                        break;
                        
                    case NAME_IMAGES:
                        filetype = TYPE_DISK;
                        break;
                        
                    case NAME_UNKNOWN:
                    case NAME_IGNORE:
                    default:
                        // TODO: what to do with unknown file types?
                        continue;
                }

                a = Archive::open(name, filetype, FILE_NOWHERE, 0);
                if (!a) {
                    ret = -1;
                }
            }
	}
	if (a && a->is_writable()) {
            if (fix_options & FIX_PRINT) {
		printf("%s: delete used file '%s'\n", a->name.c_str(), a->files[entry.index].filename().c_str());
            }
	    /* TODO: check for error */
	    a->file_delete(entry.index);
	}
    }

    if (!close_archive(a.get())) {
        ret = -1;
    }

    return ret;
}


void DeleteList::used(Archive *a, size_t index) {
    FileLocation fl(a->name + (a->contents->flags & ARCHIVE_FL_TOP_LEVEL_ONLY ? "/" : ""), index);
    
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


bool DeleteList::close_archive(Archive *archive) {
    if (archive) {
        if (!archive->commit()) {
            archive->rollback();
            return false;
        }
        
        if (archive->is_empty()) {
            remove_empty_archive(archive->name, archive->contents->flags & ARCHIVE_FL_TOP_LEVEL_ONLY);
        }
    }
    
    return true;
}
    