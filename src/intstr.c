/*
  $NiH$

  intstr.h -- map int to strings and back
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



#include <string.h>

#include "intstr.h"



const char *
int2str(int in, const intstr_t *m)
{
    int i;

    for (i=0; m[i].s; i++)
	if (m[i].i == in)
	    break;

    return m[i].s;
}



int
str2int(const char *s, const intstr_t *m)
{
    int i;

    for (i=0; m[i].s; i++)
	if (strcmp(m[i].s, s) == 0)
	    break;

    return m[i].i;
}
