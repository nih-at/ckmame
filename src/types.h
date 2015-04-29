#ifndef _HAD_TYPES_H
#define _HAD_TYPES_H

/*
  types.h -- type definitions
  Copyright (C) 1999-2015 Dieter Baron and Thomas Klausner

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


enum status {
    STATUS_OK, STATUS_BADDUMP, STATUS_NODUMP
};

typedef enum status status_t;

enum quality {
    QU_MISSING,		/* ROM is missing */
    QU_NOHASH,		/* disk and file have no common checksums */
    QU_HASHERR,		/* rom/disk and file have different checksums */
    QU_LONG,		/* long ROM with valid subsection */
    QU_NAMEERR,		/* wrong name */
    QU_COPIED,		/* copied from elsewhere */
    QU_INZIP,		/* is in zip, should be in ancestor */
    QU_OK,		/* name/size/crc match */
    QU_OLD		/* exists in old */
};

typedef enum quality quality_t;

enum file_status {
    FS_MISSING,		/* file does not exist (only used for disks) */
    FS_UNKNOWN,		/* unknown */
    FS_BROKEN,		/* file in zip broken (invalid data / crc error) */
    FS_PARTUSED,	/* part needed here, whole file unknown */
    FS_SUPERFLUOUS,	/* known, not needed here, and exists elsewhere */
    FS_NEEDED,		/* known and needed elsewhere */
    FS_USED,		/* needed here */
    FS_DUPLICATE	/* exists in old */
};

typedef enum file_status file_status_t;

enum game_status {
    GS_MISSING,		/* not a single own ROM found */
    GS_CORRECT,		/* all ROMs correct */
    GS_FIXABLE,		/* only fixable errors */
    GS_PARTIAL,		/* some ROMs missing */
    GS_OLD		/* all ROMs in old */
};

typedef enum game_status game_status_t;

enum where {
    FILE_NOWHERE = -1,
    FILE_INZIP, FILE_INCO, FILE_INGCO,
    FILE_ROMSET,
    FILE_NEEDED,
    FILE_SUPERFLUOUS,
    FILE_EXTRA,
    FILE_OLD,

    /* uncommited changes to archive */
    FILE_DELETED,
    FILE_ADDED
};

typedef enum where where_t;

#define IS_ELSEWHERE(w)	((w) >= FILE_ROMSET)
#define IS_EXTERNAL(w)	((w) > FILE_ROMSET && (w) < FILE_OLD)

#define SIZE_UNKNOWN	UINT64_MAX
#define SIZE_IS_KNOWN(s)	((s) != SIZE_UNKNOWN)

/* keep in sync with archive_modify.c:ops */
/* also, bump database version on change */
enum filetype {
    TYPE_ROM, TYPE_SAMPLE, TYPE_DISK,
    TYPE_MAX,
    /* for archive_new only */
    TYPE_FULL_PATH
};

typedef enum filetype filetype_t;

enum name_type {
    NAME_ZIP,
    NAME_CHD,
    NAME_NOEXT,
    NAME_UNKNOWN
};

typedef enum name_type name_type_t;

#define FIX_DO			0x001 /* really make fixes */
#define FIX_PRINT		0x002 /* print fixes made */
#define FIX_MOVE_LONG		0x004 /* move partially used files to
					 garbage */
#define FIX_MOVE_UNKNOWN	0x008 /* move unknown files to garbage */
#define FIX_DELETE_EXTRA	0x010 /* delete used from extra dirs */
#define FIX_CLEANUP_EXTRA	0x020 /* delete superfluous from extra dirs */ 
#define FIX_SUPERFLUOUS		0x040 /* move/delete superfluous */
#define FIX_IGNORE_UNKNOWN	0x080 /* ignore unknown files during fixing */
#define FIX_DELETE_DUPLICATE	0x100 /* delete files present in old.db */
#if 0 /* not supported (yet?) */
#define FIX_COMPLETE_GAMES	0x200 /* complete in old or complete in roms */
#endif


int filetype_db_key(filetype_t);

#endif /* types.h */
