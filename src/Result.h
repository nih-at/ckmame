#ifndef HAD_RESULT_H
#define HAD_RESULT_H

/*
  Result.h -- result of game check
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
    GS_MISSING_BEST, /* not a single own ROM found, all own ROMs marked as mia in ROM database */
    GS_CORRECT, /* all ROMs correct */
    GS_CORRECT_MIA, /* all ROMs correct, at least one marked mia in ROM database */
    GS_FIXABLE, /* only fixable errors */
    GS_PARTIAL, /* some non-MIA ROMs missing */
    GS_PARTIAL_BEST, /* only MIA ROMs are missing */
    GS_PARTIAL_MIA, /* some non-MIA ROMs missing, have at least one MIA ROM */
    GS_PARTIAL_BEST_MIA, /* only MIA ROMs are missing, have at least one MIA ROM */
    GS_OLD      /* all ROMs in old */
};

#define GS_IS_CORRECT(gs) ((gs) == GS_CORRECT || (gs) == GS_CORRECT_MIA)
#define GS_IS_MISSING(gs) ((gs) == GS_MISSING || (gs) == GS_MISSING_BEST)
#define GS_IS_PARTIAL(gs) ((gs) == GS_PARTIAL || (gs) == GS_PARTIAL_BEST || (gs) == GS_PARTIAL_MIA || (gs) == GS_PARTIAL_BEST_MIA)

#define GS_IS_BEST(gs) (!GS_HAS_MISSING(gs) || (gs) == GS_MISSING_BEST || (gs) == GS_PARTIAL_BEST || (gs) == GS_PARTIAL_BEST_MIA)
#define GS_HAS_MISSING(gs) (GS_IS_MISSING(gs) || GS_IS_PARTIAL(gs))
//#define GS_HAS_MIA(gs) ((gs) == GS_CORRECT_MIA || (gs) == GS_PARTIAL_MIA || (gs) == GS_PARTIAL_BEST_MIA)
//#define GS_HAS_MISSING_MIA(gs) ((gs) == GS_MISSING_BEST || (gs) == GS_PARTIAL_BEST || (gs) == GS_PARTIAL_BEST_MIA)
#define GS_CAN_IMPROVE(gs) ((gs) == GS_MISSING || (gs) == GS_PARTIAL || (gs) == GS_PARTIAL_MIA)

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

#endif // HAD_RESULT_H
