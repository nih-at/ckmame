/*
  $NiH$

  getline.c -- utility functions
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



#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "util.h"
#include "xmalloc.h"




char *
getline(FILE *f)
{
    static char *buf = NULL;
    static int buf_size = 0;

    int n;

    n = 0;
    for (;;) {
	if (n == buf_size) {
	    buf_size += 8192;
	    buf = xrealloc(buf, buf_size);
	}
	if (fgets(buf+n, buf_size-n, f) == NULL) {
	    if (n == 0)
		return NULL;
	    break;
	}
	n += strlen(buf+n);
	if (buf[n-1] == '\n')
	    break;
    }

    if (buf[n-1] == '\n') {
	if (buf[n-2] == '\r')
	    --n;
	--n;
    }
    buf[n] = '\0';

    return buf;
}
