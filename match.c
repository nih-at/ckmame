/*
  $NiH: match.c,v 1.32 2004/04/26 11:49:37 dillo Exp $

  match.c -- find matches
  Copyright (C) 1999, 2004 Dieter Baron and Thomas Klausner

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



#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "types.h"
#include "dbl.h"
#include "error.h"
#include "util.h"
#include "romutil.h"
#include "xmalloc.h"

extern char *prg;

static int match(struct game *game, struct zfile *zip, int zno,
		 struct match *m);
static int add_match(struct match *m, enum where where, int zno, int fno,
		     enum state st, int offset);
int matchcmp(struct match *m1, struct match *m2);

void warn_game(char *name);
void warn_rom(struct rom *rom, char *fmt, ...);
void warn_file(struct rom *r, char *fmt, ...);
void warn_disk(struct disk *d, char *fmt, ...);

static char *zname[] = {
    "zip",
    "cloneof",
    "grand-cloneof"
};



struct match *
check_game(struct game *game, struct zfile **zip)
{
    int i, zno[3];
    struct match *m;

    m = (struct match *)xmalloc(sizeof(struct match)*game->nrom);

    for (i=0; i<game->nrom; i++) {
	m[i].quality = ROM_UNKNOWN;
	m[i].next = NULL;
    }

    for (i=0; i<3; i++) {
	if (zip[i] && zip[i]->name) {
	    match(game, zip[i], i, m);
	    zno[i] = zip[i]->nrom;
	}
	else
	    zno[i] = 0;
    }

    marry(m, game->nrom, zno);

    return m;
}



void
merge_match(struct match *m, int nrom, struct zfile **zip, int pno, int gpno)
{
    int zno[3], i;
    struct match *mm;
    
    /* update zip structures */

    zno[0] = 0;
    zno[1] = pno;
    zno[2] = gpno;

    for (i=0; i<nrom; i++) {
	if (m[i].quality > ROM_UNKNOWN) {
	    if (zip[m[i].zno]->rom[m[i].fno].state < ROM_TAKEN
		|| m[i].zno == 0) {
		zip[m[i].zno]->rom[m[i].fno].state = ROM_TAKEN;
		zip[m[i].zno]->rom[m[i].fno].where = zno[m[i].zno];
	    }
	}
	for (mm=m[i].next; mm; mm=mm->next)
	    if (mm->quality > ROM_UNKNOWN
		&& mm->quality > zip[mm->zno]->rom[mm->fno].state) {
		zip[mm->zno]->rom[mm->fno].state = mm->quality;
		zip[mm->zno]->rom[mm->fno].where = zno[mm->zno];
	    }
    }

    return;
}



static int
match(struct game *game, struct zfile *zip, int zno, struct match *m)
{
    int i, j, offset;
    enum state st;
    
    for (i=0; i<game->nrom; i++) {
	for (j=0; j<zip->nrom; j++) {
	    st = romcmp(zip->rom+j, game->rom+i, zno);
	    if (st == ROM_LONG) {
		offset = findcrc(zip, j, game->rom[i].size,
				 &game->rom[i].hashes);
		if (offset != -1)
		    st = ROM_LONGOK;
	    }
	    
	    if (st != ROM_UNKNOWN)
		add_match(m+i, game->rom[i].where, zno, j, st, offset);
	}
    }

    return 0;
}



static int
add_match(struct match *m, enum where where, int zno, int fno,
	  enum state st, int offset)
{
    struct match *p, *q;

    p = (struct match *)xmalloc(sizeof(struct match));
    p->where = where;
    p->zno = zno;
    p->fno = fno;
    p->quality = st;
    p->offset = (st == ROM_LONGOK ? offset : -1);

    for (q=m; q->next; q=q->next)
	if (matchcmp(q->next, p) < 0)
	    break;

    p->next = q->next;
    q->next = p;

    return 0;
}



int
matchcmp(struct match *m1, struct match *m2)
{
    if (m1->quality == m2->quality) {
	if (m1->where == m1->zno) {
	    if (m2->where == m2->zno)
		return 0;
	    else
		return 1;
	}
	else {
	    if (m2->where == m2->zno)
		return -1;
	    else
		return 0;
	}
    }
    else
	return m1->quality - m2->quality;
}



void
diagnostics(struct game *game, struct match *m, struct disk_match *md,
	    struct zfile **zip)
{
    int i, alldead, allcorrect, allowndead, hasown;
    warn_game(game->name);

    /* analyze result: roms */
    alldead = allcorrect = allowndead = 1;
    hasown = 0;
    for (i=0; i<game->nrom; i++) {
	if (game->rom[i].where == ROM_INZIP) {
	    hasown = 1;
	    if (m[i].quality >= ROM_NAMERR) {
		alldead = allowndead = 0;
	    }
	}
	else if (m[i].quality >= ROM_NAMERR) {
	    alldead = 0;
	}
	if (!(m[i].where == game->rom[i].where
	      && m[i].quality == ROM_OK))
	    allcorrect = 0;
    }

    if (((alldead || (hasown && allowndead)) && game->nrom > 0
	 && (output_options & WARN_MISSING))) {
	warn_rom(NULL, "not a single rom found");
    }
    else if (allcorrect && (output_options & WARN_CORRECT)) {
	warn_rom(NULL, "correct");
    }
    else {
        for (i=0; i<game->nrom; i++) {
	    switch (m[i].quality) {
	    case ROM_UNKNOWN:
		if (output_options & WARN_MISSING) {
		    /* XXX */
		    if (((game->rom[i].hashes.crc != 0 && game->rom[i].flags != FLAGS_NODUMP)
			 || game->rom[i].size == 0)
			|| output_options & WARN_NO_GOOD_DUMP)
			warn_rom(game->rom+i, "missing");
		}
		break;
		
	    case ROM_SHORT:
		if (output_options & WARN_SHORT)
		    warn_rom(game->rom+i, "short (%d)",
			     zip[m[i].zno]->rom[m[i].fno].size);
		break;
		
	    case ROM_LONG:
		if (output_options & WARN_LONG)
		    warn_rom(game->rom+i,
			     "too long, unfixable (%d)",
			     zip[m[i].zno]->rom[m[i].fno].size);
		break;
		
	    case ROM_CRCERR:
		if (output_options & WARN_WRONG_CRC) {
		    warn_rom(game->rom+i, "wrong crc (%0.8x)",
			     zip[m[i].zno]->rom[m[i].fno].hashes.crc);
		}
		break;

	    case ROM_NAMERR:
		if (output_options & WARN_WRONG_NAME)
		    warn_rom(game->rom+i, "wrong name (%s)",
			     zip[m[i].zno]->rom[m[i].fno].name);
		break;
		
	    case ROM_LONGOK:
		if (output_options & WARN_LONGOK)
		    warn_rom(game->rom+i, "too long, valid subsection"
			     " at byte %d (%d)", m[i].next->offset,
			     zip[m[i].zno]->rom[m[i].fno].size);
		break;
		
	    case ROM_BESTBADDUMP:
		if (output_options & (WARN_NO_GOOD_DUMP|WARN_CORRECT))
		    warn_rom(game->rom+i, "best bad dump");
		break;
		
	    case ROM_OK:
		/* XXX */
		if ((game->rom[i].hashes.crc == 0 || game->rom[i].flags == FLAGS_NODUMP)
		    && game->rom[i].size != 0) {
		    if (output_options & WARN_NO_GOOD_DUMP)
			warn_rom(game->rom+i, "exists");
		}
		else if (game->rom[i].where == m[i].zno &&
			 (output_options & WARN_CORRECT))
		    warn_rom(game->rom+i, "correct");
		break;
		
	    default:
		break;
	    }
	    

	    if (m[i].quality != ROM_UNKNOWN
		&& game->rom[i].where != m[i].zno) {
		if (output_options & WARN_WRONG_ZIP)
		    warn_rom(game->rom+i, "should be in %s, is in %s",
			     zname[game->rom[i].where],
			     zname[m[i].zno]);
	    }
	}
    }

    /* analyze result: files */
    if (zip[0]) {
	for (i=0; i<zip[0]->nrom; i++) {
	    if ((zip[0]->rom[i].state == ROM_UNKNOWN
		 || (zip[0]->rom[i].state < ROM_NAMERR
		     && zip[0]->rom[i].where != 0))
		&& (output_options & WARN_UNKNOWN))
		warn_file(zip[0]->rom+i, "unknown");
	    else if ((zip[0]->rom[i].state < ROM_TAKEN)
		     && (output_options & WARN_NOT_USED))
		warn_file(zip[0]->rom+i, "not used");
	    else if ((zip[0]->rom[i].where != 0)
		     && (output_options & WARN_USED))
		warn_file(zip[0]->rom+i, "used in clone %s",
			  game->clone[zip[0]->rom[i].where-1]);
	}
    }

    /* analyze result: disks */

    if (md) {
	for (i=0; i<game->ndisk; i++) {
	    switch (md[i].quality) {
	    case ROM_UNKNOWN:
		if (output_options & WARN_MISSING)
		    warn_disk(game->disk+i, "missing");
		break;
		
	    case ROM_CRCERR:
		if (output_options & WARN_WRONG_CRC) {
		    /* XXX: display checksum(s) */
		    warn_disk(game->disk+i, "wrong checksum");
		}
		break;
		
	    case ROM_NOCRC:
		if (output_options & WARN_WRONG_CRC) {
		    /* XXX: display checksum(s) */
		    warn_disk(game->disk+i, "no common checksum types");
		}
		break;
		
		
	    case ROM_OK:
		if (output_options & WARN_CORRECT)
		    warn_disk(game->disk+i, "correct");
		break;
		
	    default:
		break;
	    }
	}
    }
}



static char *gname;
static int gnamedone;

void
warn_game(char *name)
{
    gname = name;
    gnamedone = 0;
}



void
warn_rom(struct rom *r, char *fmt, ...)
{
    va_list va;
    char buf[100];
    int j;

    if (gnamedone == 0) {
	printf("In game %s:\n", gname);
	gnamedone = 1;
    }

    if (r) {
	printf("rom  %-12s  ", r->name);
	if (r->size) {
	    /* XXX */
	    if (r->hashes.types & GOT_CRC) {
		if (r->flags == FLAGS_OK)
		    sprintf(buf, "size %7ld  crc %.8lx: ", r->size, r->hashes.crc);
		else if (r->flags == FLAGS_BADDUMP)
		    sprintf(buf, "size %7ld  bad dump    : ", r->size);
		else
		    sprintf(buf, "size %7ld  no good dump: ", r->size);
	    } else
		sprintf(buf, "size %7ld  no good dump: ", r->size);

	}
	else
	    sprintf(buf, "                          : ");
	printf(buf);
    }
    else
	printf("game %-40s: ", gname);
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    if (r && r->naltname) {
	for (j=0; j<r->naltname; j++) {
	    printf("rom  %-12s  ", r->altname[j]);
	    printf(buf);
	    va_start(va, fmt);
	    vprintf(fmt, va);
	    va_end(va);

	    printf(" (same as %s)\n", r->name);
	}
    }

    return;
}



void
warn_file(struct rom *r, char *fmt, ...)
{
    va_list va;

    if (gnamedone == 0) {
	printf("In game %s:\n", gname);
	gnamedone = 1;
    }

    /* XXX */
    printf("file %-12s  size %7ld  crc %.8lx: ",
	   r->name, r->size, r->hashes.crc);
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}



void
warn_disk(struct disk *d, char *fmt, ...)
{
    va_list va;

    if (gnamedone == 0) {
	printf("In game %s:\n", gname);
	gnamedone = 1;
    }

    printf("disk %-12s  ", d->name);
    if (d->hashes.types & GOT_SHA1)
	printf("sha1 %s: ", bin2hex(d->hashes.sha1, sizeof(d->hashes.sha1)));
    else if (d->hashes.types & GOT_MD5)
	printf("md5 %s         : ",
	       bin2hex(d->hashes.md5, sizeof(d->hashes.md5)));
    else
	printf("                         : ");
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}



void
match_free(struct match *m, int n)
{
    struct match *mm;
    int i;

    if (m == NULL)
	return;
    
    for (i=0; i<n; i++)
	while (m[i].next) {
	    mm = m[i].next;
	    m[i].next = mm->next;
	    free(mm);
	}

    free(m);
}



struct disk_match *
check_disks(struct game *game)
{
    struct disk_match *m;
    int i;

    if (game->ndisk == 0)
	return NULL;

    m = (struct disk_match *)xmalloc(sizeof(struct disk_match)*game->ndisk);
    
    for (i=0; i<game->ndisk; i++) {
	m[i].d.name = findfile(game->disk[i].name, TYPE_DISK);
	hashes_init(&m[i].d.hashes);

	if (m[i].d.name == NULL
	    || read_infos_from_chd(&m[i].d, diskhashtypes) < 0) {
	    m[i].quality = ROM_UNKNOWN;
	    continue;
	}

	switch (hashes_cmp(&game->disk[i].hashes, &m[i].d.hashes)) {
	case -1:
	    m[i].quality = ROM_NOCRC;
	    break;
	case 0:
	    m[i].quality = ROM_OK;
	    break;
	case 1:
	    m[i].quality = ROM_CRCERR;
	    break;
	}
    }

    return m;
}



void
disk_match_free(struct disk_match *m, int n)
{
    int i;

    if (m == NULL)
	return;
    
    for (i=0; i<n; i++)
	free(m[i].d.name);
    free(m);
}
