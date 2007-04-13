#ifndef HAD_MYINTTYPES_H
#define HAD_MYINTTYPES_H

/*
  $NiH: myinttypes.h,v 1.1 2007/04/12 16:19:56 dillo Exp $

  myinttypes.h -- ensure that {u,}int{8,16,32,64}_t are defined
  Copyright (C) 2005-2007 Dieter Baron and Thomas Klausner

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

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#else
#define bool char
#define true 1
#define false 0
#endif

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

# ifndef HAVE_INT8_T
typedef signed char int8_t;
# endif

# ifndef HAVE_UINT8_T
typedef unsigned char uint8_t;
# endif

#if !defined(HAVE_INT16_T) || !defined(HAVE_UINT16_T)
#  if SIZEOF_SHORT == 2
#    ifndef HAVE_INT16_T
typedef short int16_t;
#    endif
#    ifndef HAVE_UINT16_T
typedef unsigned short uint16_t;
#    endif
#  else
#error no 2-byte integer type found
#  endif
#endif

#if !defined(HAVE_INT32_T) || !defined(HAVE_UINT32_T)
#  if SIZEOF_INT == 4
#    ifndef HAVE_INT32_T
typedef int int32_t;
#    endif
#    ifndef HAVE_UINT32_T
typedef unsigned int uint32_t;
#    endif
#  elif SIZEOF_LONG == 4
#    ifndef HAVE_INT32_T
typedef long int32_t;
#    endif
#    ifndef HAVE_UINT32_T
typedef unsigned long uint32_t;
#    endif
#  elif SIZEOF_SHORT == 4
#    ifndef HAVE_INT32_T
typedef short int32_t;
#    endif
#    ifndef HAVE_UINT32_T
typedef unsigned short uint32_t;
#    endif
#  else
#error no 4-byte integer type found
#  endif
#endif

#if !defined(HAVE_INT64_T) || !defined(HAVE_UINT64_T)
#  if SIZEOF_INT == 8
#    ifndef HAVE_INT64_T
typedef int int64_t;
#    endif
#    ifndef HAVE_UINT64_T
typedef unsigned int uint64_t;
#    endif
#  elif SIZEOF_LONG == 8
#    ifndef HAVE_INT64_T
typedef long int64_t;
#    endif
#    ifndef HAVE_UINT64_T
typedef unsigned long uint64_t;
#    endif
#  elif SIZEOF_LONG_LONG == 8
#    ifndef HAVE_INT64_T
typedef long long int64_t;
#    endif
#    ifndef HAVE_UINT64_T
typedef unsigned long long uint64_t;
#    endif
#  else
#error no 4-byte integer type found
#  endif
#endif

#endif /* myinttypes.h */
