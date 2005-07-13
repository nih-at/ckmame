#ifndef _HAD_TYPES_H
#define _HAD_TYPES_H

/*
  $NiH: types.h,v 1.4 2005/07/07 22:00:20 dillo Exp $

  types.h -- type definitions
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



enum flags {
    FLAGS_OK, FLAGS_BADDUMP, FLAGS_NODUMP
};

typedef enum flags flags_t;

enum where {
    ROM_INZIP, ROM_INCO, ROM_INGCO
};

typedef enum where where_t;

enum state {
    ROM_0,
    ROM_UNKNOWN, ROM_SHORT, ROM_LONG, ROM_CRCERR, ROM_NOCRC,
    ROM_NAMERR, ROM_LONGOK, ROM_BESTBADDUMP, ROM_OK, ROM_TAKEN
};

typedef enum state state_t;

enum filetype {
    TYPE_ROM, TYPE_SAMPLE, TYPE_DISK,
    TYPE_MAX
};

typedef enum filetype filetype_t;



extern int output_options;
extern int fix_do, fix_print, fix_keep_long, fix_keep_unused,
    fix_keep_unknown;
extern int romhashtypes, diskhashtypes;



const char *filetype_db_key(filetype_t);


#endif /* types.h */
