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
#include <unordered_set>

#include "Dir.h"
#include "error.h"
#include "fix_util.h"
#include "fix.h"
#include "globals.h"
#include "RomDB.h"
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


void DeleteList::add_directory(const std::string &directory, bool omit_known) {
    std::unordered_set<std::string> known_games;
    
    if (omit_known) {
        auto list = db->read_list(DBH_KEY_LIST_GAME);
        if (list.empty()) {
            myerror(ERRDEF, "list of games not found in ROM database");
            exit(1);
        }
        known_games.insert(list.begin(), list.end());
    }
        
    bool have_toplevel_roms = false;
    bool have_toplevel_disks = false;

    try {
        Dir dir(directory, false);
        std::filesystem::path filepath;
        
        while ((filepath = dir.next()) != "") {
            if (name_type(filepath) == NAME_IGNORE) {
                continue;
            }
            
            bool known = false;
            
            if (std::filesystem::is_directory(filepath)) {
                auto filename = filepath.filename();
                known = known_games.find(filename) != known_games.end();
                                
                if (roms_unzipped) {
                    if (!known) {
                        archives.push_back(ArchiveLocation(filepath, TYPE_ROM));
                    }
                }
                else {
                    if (!known) {
                        archives.push_back(ArchiveLocation(filepath, TYPE_DISK));
                    }
                    list_non_chds(filepath);
                }
            }
            else {
                if (!roms_unzipped) {
                    auto ext = filepath.extension();
                    
                    if (ext == ".zip") {
                        auto stem = filepath.stem();
                        known = known_games.find(stem) != known_games.end();
                    }
                    else if (ext == ".chd") {
                        // TODO: I don't think we want top level CHDs in this list.
                        known = true;
                        have_toplevel_disks = true;
                    }
                }
                else {
                    // TODO: Don't list top level files for unzipped in list either.
                    known = true;
                    have_toplevel_roms = true;
                }

                if (!known) {
                    archives.push_back(ArchiveLocation(filepath, TYPE_ROM));
                }
            }
        }
        
        if (have_toplevel_roms) {
            archives.push_back(ArchiveLocation(directory + "/", TYPE_ROM));
        }
        if (have_toplevel_disks) {
            archives.push_back(ArchiveLocation(directory + "/", TYPE_DISK));
        }
    }
    catch (...) {
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
                a = Archive::open(name, entries[i].filetype, FILE_NOWHERE, 0);
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
    FileLocation fl(a->name + (a->contents->flags & ARCHIVE_FL_TOP_LEVEL_ONLY ? "/" : ""), a->filetype, index);
    
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
            remove_empty_archive(archive);
        }
    }
    
    return true;
}


void DeleteList::remove_archive(Archive *archive) {
    auto entry = std::find(archives.begin(), archives.end(), ArchiveLocation(archive));
    if (entry != archives.end()) {
        /* "needed" zip archives are not in list */
        archives.erase(entry);
    }
}
    
void DeleteList::sort_archives() {
    std::sort(archives.begin(), archives.end());
}


void DeleteList::sort_entries() {
    std::sort(entries.begin(), entries.end());
}


void DeleteList::list_non_chds(const std::string &directory) {
    try {
        Dir dir(directory, true);
        std::filesystem::path filepath;
        
        while ((filepath = dir.next()) != "") {
            if (filepath.extension() != ".chd") {
                archives.push_back(ArchiveLocation(filepath, TYPE_ROM));
            }
        }
    }
    catch (...) {
        return;
    }
}
