/*
  $NiH: warn.c,v 1.2 2006/09/29 16:01:34 dillo Exp $

  warn.h -- emit warning
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



#include <stdarg.h>
#include <stdio.h>

#include "warn.h"

static const char *header_name;
static int header_done;
static warn_type_t header_type;

static void warn_ensure_header(void);



void
warn_disk(const disk_t *d, const char *fmt, ...)
{
    va_list va;
    char buf[HASHES_SIZE_MAX*2 + 1];
    const hashes_t *h;

    warn_ensure_header();
    
    printf("disk %-12s  ", disk_name(d));

    h = disk_hashes(d);
    if (hashes_has_type(h, HASHES_TYPE_SHA1))
	printf("sha1 %s: ", hash_to_string(buf, HASHES_TYPE_SHA1, h));
    else if (hashes_has_type(h, HASHES_TYPE_MD5))
	printf("md5 %s         : ", hash_to_string(buf, HASHES_TYPE_MD5, h));
    else
	printf("no good dump              : ");
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}



void
warn_file(const rom_t *r, const char *fmt, ...)
{
    va_list va;

    warn_ensure_header();
    
    /* XXX */
    printf("file %-12s  size %7ld  crc %.8lx: ",
	   rom_name(r), rom_size(r), hashes_crc(rom_hashes(r)));
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}



void
warn_image(const char *name, const char *fmt, ...)
{
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
warn_rom(const rom_t *r, const char *fmt, ...)
{
    va_list va;
    char buf[100], *p;
    int j;

    warn_ensure_header();
    
    if (r) {
	printf("rom  %-12s  ", rom_name(r));
	if (SIZE_IS_KNOWN(rom_size(r))) {
	    sprintf(buf, "size %7ld  ", rom_size(r));
	    p = buf + strlen(buf);
	    
	    /* XXX */
	    if (hashes_has_type(rom_hashes(r), HASHES_TYPE_CRC)) {
		switch (rom_status(r)) {
		case STATUS_OK:
		    sprintf(p, "crc %.8lx: ", hashes_crc(rom_hashes(r)));
		    break;
		case STATUS_BADDUMP:
		    sprintf(p, "bad dump    : ");
		    break;
		case STATUS_NODUMP:
		    sprintf(p, "no good dump: ");
		}
	    } else
		sprintf(p, "no good dump: ");

	}
	else
	    sprintf(buf, "                          : ");
	fputs(buf, stdout);
    }
    else {
	/* XXX: use warn_game */
	printf("game %-40s: ", header_name);
    }
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    if (r && rom_num_altnames(r)) {
	for (j=0; j<rom_num_altnames(r); j++) {
	    printf("rom  %-12s  ", rom_altname(r, j));
	    fputs(buf, stdout);
	    va_start(va, fmt);
	    vprintf(fmt, va);
	    va_end(va);

	    printf(" (same as %s)\n", rom_name(r));
	}
    }

    return;
}



void warn_set_info(warn_type_t type, const char *name)
{
    header_type = type;
    header_name = name;
    header_done = 0;
}



static void
warn_ensure_header(void)
{
    /* keep in sync with warn_type_t in warn.h */
    static char *tname[] = {
	"archive",
	"game",
	"image"
    };

    if (header_done == 0) {
	printf("In %s %s:\n", tname[header_type], header_name);
	header_done = 1;
    }
}
