/*
  check_files.c -- match files against ROMs
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

#include "check.h"

#include "check_util.h"
#include "find.h"
#include "globals.h"
#include "RomDB.h"
#include "CkmameCache.h"


void check_archive_files(filetype_t filetype, const GameArchives &archives, const std::string &gamename, Result *result) {
    find_result_t found;
    
    auto archive = archives.archive[filetype];

    if (!archive) {
        return;
    }
    

    for (size_t i = 0; i < archive->files.size(); i++) {
        auto &file = archive->files[i];
        
        if (file.broken) {
            result->archive_files[filetype][i] = FS_BROKEN;
            continue;
        }
        
        if (result->archive_files[filetype][i] == FS_USED) {
            continue;
        }

        size_t detector_id = 0;
        found = find_in_old(filetype, &file, archive.get(), nullptr);
        if (found == FIND_EXISTS) {
            result->archive_files[filetype][i] = FS_DUPLICATE;
            continue;
        }

        found = find_in_romset(filetype, 0, &file, archive.get(), gamename, file.name, nullptr);
        if (found == FIND_UNKNOWN) {
            archive->compute_detector_hashes(db->detectors);
            for (const auto &pair : db->detectors) {
                auto id = pair.first;
                if (!file.is_size_known(id)) {
                    continue;
                }
                FileData file_data;
                file_data.name = file.name;
                file_data.hashes = file.get_hashes(id);
                auto detector_result = find_in_romset(filetype, id, &file_data, archive.get(), gamename, file.name, nullptr);
                if (detector_result != FIND_UNKNOWN) {
                    detector_id = id;
                    found = detector_result;
                    break;
                }
            }
        }

        switch (found) {
            case FIND_UNKNOWN:
                break;
                
            case FIND_EXISTS:
                result->archive_files[filetype][i] = FS_SUPERFLUOUS;
                break;
                
            case FIND_MISSING:
                if (file.hashes.size == 0) {
                    result->archive_files[filetype][i] = FS_SUPERFLUOUS;
                }
                else if (archive->where == FILE_NEEDED) {
                    /* this state does what we want, even if it sounds strange,
                     and saves us from introducing a better one */
                    result->archive_files[filetype][i] = FS_MISSING;
                }
                else {
                    Match match;
		    ckmame_cache->ensure_needed_maps();
                    if (find_in_archives(filetype, detector_id, &file, &match, true) != FIND_EXISTS) {
                        result->archive_files[filetype][i] = FS_NEEDED;
                    }
                    else {
                        result->archive_files[filetype][i] = FS_SUPERFLUOUS;
                    }
                }
                break;
                
            case FIND_ERROR:
                /* TODO: how to handle? */
                break;
        }
    }
}


void check_needed_files(filetype_t filetype, const ArchivePtr& archive, Result *result) {
    find_result_t found;
    
    if (!archive) {
        return;
    }
    

    for (size_t i = 0; i < archive->files.size(); i++) {
        auto &file = archive->files[i];
        
        if (file.broken) {
            result->archive_files[filetype][i] = FS_BROKEN;
            continue;
        }
        
        if (result->archive_files[filetype][i] == FS_USED) {
            continue;
        }

        found = find_in_old(filetype, &file, archive.get(), nullptr);
        if (found == FIND_EXISTS) {
            // TODO: check that it also exists in ROM DB
            if (configuration.keep_old_duplicate) {
                result->archive_files[filetype][i] = FS_NEEDED;
            }
            else {
                result->archive_files[filetype][i] = FS_DUPLICATE;
            }
            continue;
        }

        found = find_in_romset(filetype, 0, &file, archive.get(), "", file.name, nullptr);
        if (found == FIND_UNKNOWN) {
            archive->compute_detector_hashes(db->detectors);
            for (const auto &pair : db->detectors) {
                auto id = pair.first;
                if (!file.is_size_known(id)) {
                    continue;
                }
                FileData file_data;
                file_data.name = file.name;
                file_data.hashes = file.get_hashes(id);
                auto detector_result = find_in_romset(filetype, id, &file_data, archive.get(), "", file.name, nullptr);
                if (detector_result != FIND_UNKNOWN) {
                    found = detector_result;
                    break;
                }
            }
        }

        switch (found) {
            case FIND_UNKNOWN:
                break;
                
            case FIND_EXISTS:
                result->archive_files[filetype][i] = FS_SUPERFLUOUS;
                break;
                
            case FIND_MISSING:
                result->archive_files[filetype][i] = FS_NEEDED;
                break;
                
            case FIND_ERROR:
                /* TODO: how to handle? */
                break;
        }
    }
}
