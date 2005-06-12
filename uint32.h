#ifndef HAD_UINT32_H
#define HAD_UINT32_H

/*
  $NiH: uint32.h,v 1.1 2005/06/12 17:09:28 dillo Exp $

  uint32.h -- ensure that uint32_t is defined to a 4-byte type
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

#ifndef HAVE_UINT32_T
# ifdef HAVE_STDINT_H
#include <stdint.h>
# elif defined(HAVE_INTTYPES_H)
#incldue <inttypes.h>
# else
#  if SIZEOF_UNSIGNED_INT == 4
typedef unsigned int uint32_t
#  elif SIZEOF_UNSIGNED_LONG == 4
typedef unsigned long uint32_t
#  else
#error no 4-byte unsigned integer type found
#  endif
# endif /* !HAVE_STDINT_H && !HAVE_INTTYPES_H */
#endif /* HAVE_UINT32_t */

#endif /* uint32.h */
