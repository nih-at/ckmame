/*
  $NiH$

  getprogname.c -- getprogname/setprogname replacemenet
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

static const char *progname;



const char *
getprogname(void)
{
    return progname;
}



void
setprogname(const char *name)
{
    if ((progname=strrchr(name, '/')) == NULL)
	progname = name;
    else
	progname++;
}
