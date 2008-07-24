#ifndef HAD_MYINTTYPES_H
#define HAD_MYINTTYPES_H

/*
  $NiH: myinttypes.h,v 1.1 2007/04/12 16:19:56 dillo Exp $

  myinttypes.h -- ensure that {u,}int{8,16,32,64}_t and PRIu64 are defined
  Copyright (C) 2005-2008 Dieter Baron and Thomas Klausner

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

#if !defined(HAVE_INT64_T) || !defined(HAVE_UINT64_T) || !defined(PRIu64)
#  if SIZEOF_INT == 8
#    ifndef HAVE_INT64_T
typedef int int64_t;
#    endif
#    ifndef HAVE_UINT64_T
typedef unsigned int uint64_t;
#    endif
#    ifndef PRIu64
#define PRIu64 "u"
#    endif
#  elif SIZEOF_LONG == 8
#    ifndef HAVE_INT64_T
typedef long int64_t;
#    endif
#    ifndef HAVE_UINT64_T
typedef unsigned long uint64_t;
#    endif
#    ifndef PRIu64
#define PRIu64 "ul"
#    endif
#  elif SIZEOF_LONG_LONG == 8
#    ifndef HAVE_INT64_T
typedef long long int64_t;
#    endif
#    ifndef HAVE_UINT64_T
typedef unsigned long long uint64_t;
#    endif
#    ifndef PRIu64
#define PRIu64 "ull"
#    endif
#  else
#error no 4-byte integer type found
#  endif
#endif

#endif /* myinttypes.h */
