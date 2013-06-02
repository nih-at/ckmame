/*
  error.c -- error printing
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

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



#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "dbh.h"
#include "error.h"

#define DEFAULT_FN	"<unknown>"

static const char *myerrorfn = DEFAULT_FN;
static const char *myerrorzipn = DEFAULT_FN;
static dbh_t *myerrdb = NULL;



void
myerror(int errtype, const char *fmt, ...)
{
    va_list va;

    fprintf(stderr, "%s: ", getprogname());

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
    if (errtype & ERRDB) {
	if (myerrdb == NULL)
	    fprintf(stderr, ": no database");
	else
	    fprintf(stderr, ": %s", dbh_error(myerrdb));
    }
    
    putc('\n', stderr);

    return;
}



void
seterrdb(dbh_t *db)
{
    myerrdb = db;
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

