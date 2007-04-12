#ifndef HAD_BSWAP_H
#define HAD_BSWAP_H

/*
  $NiH$

  bswap.h -- byte swap definitions
  Copyright (C) 2007 Dieter Baron and Thomas Klausner

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



#include "config.h"

#ifdef HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif

#include "myinttypes.h"



#ifndef HAVE_BSWAP16
uint16_t bswap16(uint16_t);
#endif

#ifndef HAVE_BSWAP32
uint32_t bswap32(uint32_t);
#endif

#endif /* bswap.h */
