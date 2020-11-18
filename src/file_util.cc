/*
  file_util.c -- utility functions for manipulating files
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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
#include <stdio.h>
#include <unistd.h>

#include "error.h"
#include "hashes.h"

int
copy_file(const char *old, const char *new_name, size_t start, ssize_t len, hashes_t *hashes) {
    FILE *fin, *fout;
    unsigned char b[8192];
    size_t nr, nw, total;
    ssize_t n;
    int err;

    if ((fin = fopen(old, "rb")) == NULL)
	return -1;

    if (start > 0) {
	if (fseeko(fin, start, SEEK_SET) < 0) {
	    fclose(fin);
	    return -1;
	}
    }

    if ((fout = fopen(new_name, "wb")) == NULL) {
	fclose(fin);
	return -1;
    }

    hashes_update_t *hu = NULL;
    hashes_t h;

    if (hashes) {
	hashes_types(&h) = HASHES_TYPE_ALL;
	hu = hashes_update_new(&h);
    }

    total = 0;
    while ((len >= 0 && total < (size_t)len) || !feof(fin)) {
	nr = sizeof(b);
	if (len > 0 && nr > (size_t)len - total)
	    nr = (size_t)len - total;
	if ((nr = fread(b, 1, nr, fin)) == 0)
	    break;
	nw = 0;
	while (nw < nr) {
	    if ((n = fwrite(b + nw, 1, nr - nw, fout)) <= 0) {
		err = errno;
		fclose(fin);
		fclose(fout);
		if (remove(new_name) != 0) {
		    myerror(ERRSTR, "cannot clean up temporary file '%s' during copy error", new_name);
		}
		errno = err;
		hashes_update_discard(hu);
		return -1;
	    }

	    if (hashes)
		hashes_update(hu, b + nw, nr - nw);
	    nw += n;
	}
	total += nw;
    }

    if (hashes)
	hashes_update_final(hu);

    if (fclose(fout) != 0 || ferror(fin)) {
	err = errno;
	fclose(fin);
	if (remove(new_name) != 0) {
	    myerror(ERRSTR, "cannot clean up temporary file '%s' during copy error", new_name);
	}
	errno = err;
	return -1;
    }
    fclose(fin);

    if (hashes)
	hashes_copy(hashes, &h);

    return 0;
}


int
link_or_copy(const char *old, const char *new_name) {
    if (link(old, new_name) < 0) {
	if (copy_file(old, new_name, 0, -1, NULL) < 0) {
	    seterrinfo(old, "");
	    myerror(ERRFILESTR, "cannot link to '%s'", new_name);
	    return -1;
	}
    }

    return 0;
}


int
my_remove(const char *name) {
    if (remove(name) != 0) {
	seterrinfo(name, "");
	myerror(ERRFILESTR, "cannot remove");
	return -1;
    }

    return 0;
}


int
rename_or_move(const char *old, const char *new_name) {
    if (rename(old, new_name) < 0) {
	if (copy_file(old, new_name, 0, -1, NULL) < 0) {
	    seterrinfo(old, "");
	    myerror(ERRFILESTR, "cannot rename to '%s'", new_name);
	    return -1;
	}
	unlink(old);
    }

    return 0;
}
