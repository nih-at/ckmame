#ifndef _HAD_WARN_H
#define _HAD_WARN_H

/*
  $NiH: warn.h,v 1.3 2006/05/07 11:47:26 dillo Exp $

  warn.h -- emit warning
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include "disk.h"
#include "rom.h"



#define WARN_UNKNOWN		0x0001
#define WARN_USED		0x0002
#define WARN_NOT_USED		0x0004
#define WARN_FILE_BROKEN	0x0008

#define WARN_SUPERFLUOUS	(WARN_UNKNOWN|WARN_USED|WARN_NOT_USED)

#define WARN_WRONG_ZIP		0x0010
#define WARN_WRONG_NAME		0x0020
#define WARN_LONGOK		0x0040
#define WARN_ELSEWHERE		0x0080

#define WARN_FIXABLE		(WARN_WRONG_ZIP|WARN_WRONG_NAME|WARN_LONGOK\
				 |WARN_ELSEWHERE)

#define WARN_WRONG_CRC		0x0100
#define WARN_LONG		0x0200
#define WARN_SHORT		0x0400
#define WARN_MISSING		0x0800
#define WARN_NO_GOOD_DUMP       0x1000

#define WARN_BROKEN		(WARN_WRONG_CRC|WARN_LONG|WARN_SHORT\
				 |WARN_MISSING|WARN_NO_GOOD_DUMP)

#define WARN_ALL		(WARN_SUPERFLUOUS|WARN_FIXABLE|WARN_BROKEN\
				 |WARN_FILE_BROKEN)

#define WARN_CORRECT		0x2000

/* keep in sync with tname in warn.c:warn_ensure_header() */
enum warn_type {
    WARN_TYPE_ARCHIVE,
    WARN_TYPE_GAME,
    WARN_TYPE_IMAGE
};    

typedef enum warn_type warn_type_t;



void warn_disk(const disk_t *, const char *, ...);
void warn_file(const rom_t *, const char *, ...);
void warn_image(const char *, const char *, ...);
void warn_rom(const rom_t *, const char *, ...);
void warn_set_info(warn_type_t, const char *);

#endif /* _HAD_WARN_H */
