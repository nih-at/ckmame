#ifndef HAD_COMPAT_H
#define HAD_COMPAT_H

/*
  $NiH$

  compat.h -- prototypes/defines for missing library functions
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

#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#else
#include <compat_fnmatch.h>
#endif

#ifndef HAVE_FSEEKO
#define fseeko fseek
#endif

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include <compat_getopt.h>
#endif

#ifndef HAVE_GETPROGNAME
const char *getprogname(void);
void setprogname(const char *);
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif

#endif /* compat.h */
