#ifndef HAD_RESULT_H
#define HAD_RESULT_H

/*
  result.h -- result of game check
  Copyright (C) 2006-2021 Dieter Baron and Thomas Klausner

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

#include <vector>

#include "Game.h"
#include "GameArchives.h"
#include "Match.h"

enum GameStatus {
    GS_MISSING, /* not a single own ROM found */
    GS_CORRECT, /* all ROMs correct */
    GS_FIXABLE, /* only fixable errors */
    GS_PARTIAL, /* some ROMs missing */
    GS_OLD      /* all ROMs in old */
};

enum FileStatus {
    FS_MISSING,     /* file does not exist (only used for disks) */
    FS_UNKNOWN,     /* unknown */
    FS_BROKEN,      /* file in zip broken (invalid data / crc error) */
    FS_PARTUSED,    /* part needed here, whole file unknown */
    FS_SUPERFLUOUS, /* known, not needed here, and exists elsewhere */
    FS_NEEDED,      /* known and needed elsewhere */
    FS_USED,        /* needed here */
    FS_DUPLICATE    /* exists in old */
};

class Result {
public:
    Result(const Game *g, const GameArchives &a);
    
    GameStatus game;
    
    std::vector<Match> game_files[TYPE_MAX];
    std::vector<FileStatus> archive_files[TYPE_MAX];
};

#endif /* result.h */
