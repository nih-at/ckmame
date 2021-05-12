#ifndef _HAD_TYPES_H
#define _HAD_TYPES_H

/*
  types.h -- type definitions
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

enum quality {
    QU_MISSING, /* ROM is missing */
    QU_NOHASH,  /* disk and file have no common checksums */
    QU_LONG,    /* long ROM with valid subsection */
    QU_NAMEERR, /* wrong name */
    QU_COPIED,  /* copied from elsewhere */
    QU_INZIP,   /* is in zip, should be in ancestor */
    QU_OK,      /* name/size/crc match */
    QU_OLD      /* exists in old */
};

typedef enum quality quality_t;

enum where {
    FILE_NOWHERE = -1,
    FILE_INGAME,
    FILE_IN_CLONEOF,
    FILE_IN_GRAND_CLONEOF,
    FILE_ROMSET,
    FILE_NEEDED,
    FILE_SUPERFLUOUS,
    FILE_EXTRA,
    FILE_OLD,
};

typedef enum where where_t;

#define IS_ELSEWHERE(w) ((w) >= FILE_ROMSET)
#define IS_EXTERNAL(w) ((w) > FILE_ROMSET && (w) < FILE_OLD)

/* bump database version on change */
enum filetype {
    TYPE_ROM,
    TYPE_DISK,
    TYPE_MAX,
    /* for archive_new only */
    TYPE_FULL_PATH
};

typedef enum filetype filetype_t;

enum name_type { NAME_ZIP, NAME_IMAGES, NAME_IGNORE, NAME_UNKNOWN };

typedef enum name_type name_type_t;

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

#endif /* types.h */
