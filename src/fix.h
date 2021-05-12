#ifndef _HAD_FIX_H
#define _HAD_FIX_H

/*
  fix.h -- fix ROM sets
  Copyright (C) 1999-2021 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to fix rom sets for MAME.
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

#include "GameArchives.h"
#include "Result.h"

#define FIX_DO 0x001    /* really make fixes */
#define FIX_PRINT 0x002 /* print fixes made */
#define FIX_MOVE_LONG 0x004 /* move partially used files to garbage */
#define FIX_MOVE_UNKNOWN 0x008     /* move unknown files to garbage */
#define FIX_DELETE_EXTRA 0x010     /* delete used from extra dirs */
#define FIX_CLEANUP_EXTRA 0x020    /* delete superfluous from extra dirs */
#define FIX_SUPERFLUOUS 0x040      /* move/delete superfluous */
#define FIX_IGNORE_UNKNOWN 0x080   /* ignore unknown files during fixing */
#define FIX_DELETE_DUPLICATE 0x100 /* delete files present in old.db */
#define FIX_COMPLETE_ONLY 0x200    /* only keep complete sets in rom-dir */
#if 0                              /* not supported (yet?) */
#define FIX_COMPLETE_GAMES 0x400   /* complete in old or complete in roms */
#endif

int fix_game(Game *game, const GameArchives archives, Result *result);

extern int fix_options;

#endif /* _HAD_FIX_H */
