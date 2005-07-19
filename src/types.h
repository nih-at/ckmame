#ifndef _HAD_TYPES_H
#define _HAD_TYPES_H

/*
  $NiH: types.h,v 1.5.2.1 2005/07/15 10:02:59 dillo Exp $

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



/* XXX: rename to status */
enum flags {
    FLAGS_OK, FLAGS_BADDUMP, FLAGS_NODUMP
};

typedef enum flags flags_t;

enum quality {
    QU_MISSING,		/* ROM is missing */
    QU_LONG,		/* long ROM with valid subsection */
    QU_NAMEERR,		/* wrong name */
    QU_COPIED,		/* copied from elsewhere */
    QU_INZIP,		/* is in zip, should be in ancestor */
    QU_OK,		/* name/size/crc match */
    QU_ANCESTOR_OK	/* ancestor ROM found in ancestor */
};

typedef enum quality quality_t;

enum where {
    ROM_INZIP, ROM_INCO, ROM_INGCO
};

typedef enum where where_t;

/* XXX: delete */
enum state {
    ROM_0,
    ROM_UNKNOWN, ROM_SHORT, ROM_LONG, ROM_CRCERR, ROM_NOCRC,
    ROM_NAMERR, ROM_LONGOK, ROM_BESTBADDUMP, ROM_OK, ROM_TAKEN
};

typedef enum state state_t;

enum filetype {
    TYPE_ROM, TYPE_SAMPLE, TYPE_DISK,
    TYPE_MAX,
    /* for archive_new only */
    TYPE_FULL_PATH
};

typedef enum filetype filetype_t;






const char *filetype_db_key(filetype_t);

#endif /* types.h */
