#ifndef _HAD_ERROR_H
#define _HAD_ERROR_H

/*
  error.h -- error printing
  Copyright (C) 1999-2013 Dieter Baron and Thomas Klausner

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



#include "dbh.h"

#define ERRDEF	0x0	/* no additional info */
#define ERRZIP	0x1	/* prepend zipfile name */
#define ERRFILE	0x2	/* prepend file name */
#define ERRSTR	0x4	/* append strerror(errno) */
#define ERRDB	0x8	/* append dbh_error() */

#define ERRZIPFILE	(ERRZIP|ERRFILE)
#define	ERRZIPSTR	(ERRZIP|ERRSTR)
#define ERRFILESTR	(ERRFILE|ERRSTR)
#define ERRZIPFILESTR	(ERRZIPFILE|ERRSTR)

void myerror(int, const char *, ...);
void seterrdb(dbh_t *);
void seterrinfo(const char *, const char *);

#endif /* error.h */
