#ifndef HAD_MATCH_H
#define HAD_MATCH_H

/*
  match.h -- matching files with ROMs
  Copyright (C) 1999-2021 Dieter Baron and Thomas Klausner

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

#include <string>

#include "Archive.h"
#include "types.h"

class Match {
public:
    enum Quality {
        MISSING, /* ROM is missing */
        NO_HASH,  /* disk and file have no common checksums */
        LONG,    /* long ROM with valid subsection */
        NAME_ERROR, /* wrong name */
        COPIED,  /* copied from elsewhere */
        IN_ZIP,   /* is in zip, should be in ancestor */
        OK,      /* name/size/crc match */
        OK_AND_OLD, /* exists in ROM set and old */
        OLD,      /* exists in old */
    };
    
    Match() : quality(MISSING), where(FILE_NOWHERE), index(0), offset(0) { }
    
    Quality quality;
    where_t where;
    
    ArchivePtr archive;
    uint64_t index;

    /* for where == old */
    std::string old_game;
    std::string old_file;

    uint64_t offset; /* offset of correct part if quality == LONG */
    
    std::string game() const;
    bool source_is_old() const { return where == FILE_OLD; }
    std::string file() const;
};

#endif // HAD_MATCH_H
