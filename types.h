#ifndef _HAD_TYPES_H
#define _HAD_TYPES_H

/*
  $NiH: types.h,v 1.16 2002/06/06 09:27:00 dillo Exp $

  types.h -- type definitions
  Copyright (C) 1999 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include "zip.h"

#define WARN_CORRECT		0x1000

#define WARN_UNKNOWN		0x0001
#define WARN_USED		0x0002
#define WARN_NOT_USED		0x0004

#define WARN_SUPERFLUOUS	(WARN_UNKNOWN|WARN_USED|WARN_NOT_USED)

#define WARN_WRONG_ZIP		0x0008
#define WARN_WRONG_NAME		0x0010
#define WARN_LONGOK		0x0080

#define WARN_FIXABLE		(WARN_WRONG_ZIP|WARN_WRONG_NAME|WARN_LONGOK)

#define WARN_WRONG_CRC		0x0020
#define WARN_LONG		0x0040
#define WARN_SHORT		0x0100
#define WARN_MISSING		0x0200
#define WARN_NO_GOOD_DUMP       0x0400

#define WARN_BROKEN		(WARN_WRONG_CRC|WARN_LONG|WARN_SHORT\
				 |WARN_MISSING|WARN_NO_GOOD_DUMP)

#define WARN_ALL		(WARN_SUPERFLUOUS|WARN_FIXABLE|WARN_BROKEN)



enum where {
    ROM_INZIP, ROM_INCO, ROM_INGCO
};

enum state {
    ROM_0,
    ROM_UNKNOWN, ROM_SHORT, ROM_LONG, ROM_CRCERR,
    ROM_NAMERR, ROM_LONGOK, ROM_BESTBADDUMP, ROM_OK, ROM_TAKEN
};

struct rom {
    char *name, *merge;
    unsigned long size, crc;
    enum state state;
    enum where where;
    int naltname;
    char **altname;
};
    
struct game {
    char *name;
    char *description;
    char *cloneof[2];
    int nclone;
    char **clone;
    struct rom *rom;
    int nrom;
    char  *sampleof[2];
    int nsclone;
    char **sclone;
    struct rom *sample;
    int nsample;
};

struct match {
    struct match *next;
    enum where where;
    int zno;
    int fno;
    enum state quality;
    int offset;              /* offset of correct part if ROM_LONGOK */
};

struct zfile {
    char *name;
    struct rom *rom;
    int nrom;
    struct zip *zf;
};

struct tree {
    char *name;
    int check;
    struct tree *next, *child;
};



extern int output_options;
extern int fix_do, fix_print, fix_keep_long, fix_keep_unused,
    fix_keep_unknown;

#endif /* types.h */
