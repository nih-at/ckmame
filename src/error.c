/*
  $NiH: error.c,v 1.1 2005/07/04 21:54:50 dillo Exp $

  error.c -- error printing
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "error.h"
#include "dbh.h"

#include <errno.h>

#define DEFAULT_FN	"<unknown>"

extern char *prg;

static const char *myerrorfn = DEFAULT_FN;
static const char *myerrorzipn = DEFAULT_FN;



void
myerror(int errtype, const char *fmt, ...)
{
    va_list va;

    fprintf(stderr, "%s: ", prg);

    if ((errtype & ERRZIPFILE) == ERRZIPFILE)
	fprintf(stderr, "%s (%s): ", myerrorfn, myerrorzipn);
    else if (errtype & ERRZIP)
	    fprintf(stderr, "%s: ", myerrorzipn);
    else if (errtype & ERRFILE)
	    fprintf(stderr, "%s: ", myerrorfn);

    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);

    if ((errno != 0) && (errtype & ERRSTR))
	fprintf(stderr, ": %s", strerror(errno));
    if (errtype & ERRDB)
	fprintf(stderr, ": %s", dbh_error());
    
    putc('\n', stderr);

    return;
}



void
seterrinfo(const char *fn, const char *zipn)
{
    if (fn)
	myerrorfn = fn;
    else
	myerrorfn = DEFAULT_FN;

    if (zipn)
	myerrorzipn = zipn;
    else 
	myerrorzipn = DEFAULT_FN;
    
    return;
}

