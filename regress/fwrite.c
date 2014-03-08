/*
  fwrite.c -- override fwrite() to allow testing special cases
  Copyright (C) 2013-2014 Dieter Baron and Thomas Klausner

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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#define __USE_GNU
#include <dlfcn.h>
#undef __USE_GNU

static int inited = 0;
static size_t count = 0;
static size_t max_write = 0;
static size_t(*real_fwrite)(const void *ptr, size_t size, size_t nmemb, FILE * stream) = NULL;

static FILE *log;
static const char *myname = NULL;

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE * stream)
{
    size_t ret;

    if (!inited) {
	char *foo;
	myname = getprogname();
        log = fopen("/tmp/fwrite.log", "a");
	if (!myname)
	    myname = "(unknown)";
	if ((foo=getenv("FWRITE_MAX_WRITE")) != NULL)
	    max_write = strtoul(foo, NULL, 0);
	fprintf(log, "%s: max_write set to %lu\n", myname, max_write);
	real_fwrite = dlsym(RTLD_NEXT, "fwrite");
	if (!real_fwrite)
	    abort();
	inited = 1;
    }
 
    if (count + size*nmemb > max_write) {
	fprintf(log, "%s: returned ENOSPC\n", myname);
	errno = ENOSPC;
	return -1;
    }
    

    ret = real_fwrite(ptr, size, nmemb, stream);
    count += ret * size;

    fprintf(log, "%s: wrote %lu*%lu = %lu bytes, sum %lu\n", myname, ret, size, ret * size, count);
    return ret;
}
