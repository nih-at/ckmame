#ifndef _HAD_UTIL_H
#define _HAD_UTIL_H

/*
  $NiH: util.h,v 1.17 2005/06/12 22:32:36 wiz Exp $

  util.h -- miscellaneous utility functions
  Copyright (C) 1999, 2004, 2005 Dieter Baron and Thomas Klausner

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

#include "types.h"

typedef int (*cmpfunc)(const void *, const void *);

extern char *rompath[];
void init_rompath(void);

const char *bin2hex(const unsigned char *, int);
char *findfile(const char *, enum filetype);
int hex2bin(unsigned char *, const char *, int);
void *memdup(const void *, int);
int strpcasecmp(const char * const *, const char * const *);

#endif
