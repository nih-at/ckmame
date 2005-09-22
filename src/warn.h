#ifndef _HAD_WARN_H
#define _HAD_WARN_H

/*
  $NiH: warn.h,v 1.1.2.1 2005/07/27 00:05:58 dillo Exp $

  warn.h -- warning type definitions
  Copyright (C) 1999, 2004 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

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

#endif /* _HAD_WARN_H */
