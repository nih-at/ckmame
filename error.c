/*
  $NiH$

  error.c -- error printing
  Copyright (C) 1999 Dieter Baron and Thomas Klaunser

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "error.h"

#include <errno.h>

extern char *prg;
static char *myerrorfn, *myerrorzipn;



void
myerror(int errtype, char *fmt, ...)
{
    va_list va;

    fprintf(stderr, "%s: ", prg);

    if (((errtype==ERRFILE)||(errtype==ERRSTR)) && myerrorfn) {
	if (myerrorzipn)
	    fprintf(stderr, "%s (%s): ", myerrorfn, myerrorzipn);
	else
	    fprintf(stderr, "%s: ", myerrorfn);
    }

    if (((errtype==ERRZIP)||(errtype==ERRZIPSTR)) && myerrorzipn)
	fprintf(stderr, "%s: ", myerrorzipn);
    
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);

    if ((errno != 0) && (((errtype==ERRSTR) && (!myerrorzipn)) ||
	((errtype==ERRZIPSTR) && (myerrorzipn))))
	fprintf(stderr, ": %s", strerror(errno));
    
    putc('\n', stderr);

    return;
}



void
seterrinfo(char *fn, char *zipn)
{
    myerrorfn = fn;
    myerrorzipn = zipn;
    
    return;
}

