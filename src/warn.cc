/*
  warn.h -- emit warning
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


#include <stdarg.h>
#include <stdio.h>

#include "warn.h"

static const char *header_name;
static int header_done;
static warn_type_t header_type;

static void warn_ensure_header(void);


void
warn_disk(const Disk *disk, const char *fmt, ...) {
    va_list va;
    const Hashes *h;

    warn_ensure_header();

    printf("disk %-12s  ", disk->name.c_str());

    h = disk_hashes(disk);
    if (h->has_type(Hashes::TYPE_SHA1)) {
        printf("sha1 %s: ", h->to_string(Hashes::TYPE_SHA1).c_str());
    }
    else if (h->has_type(Hashes::TYPE_MD5)) {
        printf("md5 %s         : ", h->to_string(Hashes::TYPE_MD5).c_str());
    }
    else {
	printf("no good dump              : ");
    }

    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}


void
warn_file(const File *r, const char *fmt, ...) {
    va_list va;

    warn_ensure_header();

    /* TODO */
    printf("file %-12s  size %7" PRIu64 "  crc %.8" PRIx32 ": ", r->name.c_str(), r->size, r->hashes.crc);

    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}


void
warn_image(const char *name, const char *fmt, ...) {
    va_list va;

    warn_ensure_header();

    printf("image %-12s: ", name);

    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}


void
warn_rom(const File *r, const char *fmt, ...) {
    va_list va;
    char buf[100], *p;

    warn_ensure_header();

    if (r) {
	printf("rom  %-12s  ", r->name.c_str());
	if (r->is_size_known()) {
            sprintf(buf, "size %7" PRIu64 "  ", r->size);
	    p = buf + strlen(buf);

	    /* TODO */
	    if (r->hashes.has_type(Hashes::TYPE_CRC)) {
		switch (r->status) {
		case STATUS_OK:
		    sprintf(p, "crc %.8" PRIx32 ": ", r->hashes.crc);
		    break;
		case STATUS_BADDUMP:
		    sprintf(p, "bad dump    : ");
		    break;
		case STATUS_NODUMP:
		    sprintf(p, "no good dump: ");
		}
	    }
	    else
		sprintf(p, "no good dump: ");
	}
	else
	    sprintf(buf, "                          : ");
	fputs(buf, stdout);
    }
    else {
	/* TODO: use warn_game */
	printf("game %-40s: ", header_name);
    }

    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}


void
warn_set_info(warn_type_t type, const char *name) {
    header_type = type;
    header_name = name;
    header_done = 0;
}


static void
warn_ensure_header(void) {
    /* keep in sync with warn_type_t in warn.h */
    static const char *tname[] = {"archive", "game", "image"};

    if (header_done == 0) {
	printf("In %s %s:\n", tname[header_type], header_name);
	header_done = 1;
    }
}
