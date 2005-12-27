#ifndef HAD_PRI_H
#define HAD_PRI_H

/*
  $NiH: pri.h,v 1.1 2005/12/26 14:34:19 dillo Exp $

  pri.h -- define macros to portably print types
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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

#include "config.h"

#ifndef PRIdoff
#if SIZEOF_LONG == SIZEOF_OFF_T
#define PRIdoff "ld"
#elif SIZEOF_INT == SIZEOF_OFF_T
#define PRIdoff	"d"
#elif SIZEOF_LONG_LONG != 0 && SIZEOF_LONG_LONG == SIZEOF_OFF_T
#define PRIdoff "lld"
#else
#define PRIdoff "ld"
#define PRIoff_cast (long)
#endif
#endif

#ifndef PRIoff_cast
#define PRIoff_cast
#endif

#endif /* pri.h */
