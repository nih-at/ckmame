#ifndef HAD_FIND_H
#define HAD_FIND_H

/*
  find.h -- find ROM in ROM set or archives
  Copyright (C) 2005-2020 Dieter Baron and Thomas Klausner

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


#include "FileData.h"
#include "Match.h"

enum find_result { FIND_ERROR = -1, FIND_UNKNOWN, FIND_MISSING, FIND_EXISTS };

typedef enum find_result find_result_t;

std::vector<CkmameDB::FindResult> find_candidates_in_archives(filetype_t filetype, size_t detector_id, const FileData *rom, bool needed_only);
find_result_t check_archive_candidates(const std::vector<CkmameDB::FindResult>& candidates, filetype_t filetype, const FileData *rom, Match *match, bool needed_only);

find_result_t find_in_archives(filetype_t filetype, size_t detector_id, const FileData *r, Match *m, bool needed_only);
find_result_t find_in_old(filetype_t filetype, const FileData *file, Archive *archive, Match *match);
find_result_t find_in_romset(filetype_t ft, size_t detector_id, const FileData *file, Archive *archive, const std::string &skip_game, const std::string &skip_file, Match *match);

find_result_t check_for_file_in_archive(filetype_t filetype, size_t detector_id, const std::string &name, const FileData *wanted_file, const FileData *candidate, Match *matches);

#endif // HAD_FIND_H
