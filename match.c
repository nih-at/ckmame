#include <stdio.h>
#include <stdarg.h>

#include "types.h"
#include "dbl.h"

extern char *prg;

static int match(struct game *game, struct zip *zip, int zno, struct match *m);
static int add_match(struct match *m, enum where where, int zno, int fno,
		     enum state st);
int matchcmp(struct match *m1, struct match *m2);

void warn_rom(struct rom *rom, char *fmt, ...);
void warn_file(struct rom *r, char *fmt, ...);

static char *zname[] = {
    "zip file",
    "cloneof",
    "grand-cloneof"
};



int
check_game(DB *db, char *name)
{
    enum state st;
    int i, j, zno[3], needed;
    struct match *m;
    struct zip zip[3];
    struct game *game;

    if ((game=r_game(db, name)) == NULL)
	return -1;

    m = (struct match *)xmalloc(sizeof(struct match)*game->nrom);

    for (i=0; i<game->nrom; i++) {
	m[i].quality = ROM_UNKNOWN;
	m[i].next = NULL;
    }

    zip[0].name = findzip(game->name);
    for (i=0; i<2; i++) {
	if (game->cloneof[i])
	    zip[1+i].name = findzip(game->cloneof[i]);
	else
	    zip[1+i].name = NULL;
    }

    for (i=0; i<3; i++) {
	if (zip[i].name) {
	    zip[i].nrom = readinfosfromzip(&zip[i].rom, zip[i].name);
	    if (zip[i].nrom < 0)
		zip[i].nrom = 0;
	    else
		match(game, zip+i, i, m);
	}
	else
	    zip[i].nrom = 0;
	zno[i] = zip[i].nrom;
    }

    marry(m, game->nrom, zno);

    /* analyze result: roms */
    for (i=0; i<game->nrom; i++) {
	switch (m[i].quality) {
	case ROM_UNKNOWN:
	    warn_rom(game->rom+i, "missing");
	    break;

	case ROM_SHORT:
	    warn_rom(game->rom+i, "short (%d)",
		     zip[m[i].zno].rom[m[i].fno].size);
	    break;

	case ROM_LONG:
	    warn_rom(game->rom+i, "too long, truncating won't help (%d)",
		 zip[m[i].zno].rom[m[i].fno].size);
	    break;

	case ROM_CRCERR:
	    warn_rom(game->rom+i, "wrong crc (%0.8x)",
		 zip[m[i].zno].rom[m[i].fno].crc);
	    break;

	case ROM_NAMERR:
	    warn_rom(game->rom+i, "wrong name (%s)",
		 zip[m[i].zno].rom[m[i].fno].name);
	    break;

	case ROM_LONGOK:
	    warn_rom(game->rom+i, "too long, truncating fixes (%d)",
		 zip[m[i].zno].rom[m[i].fno].size);
	    break;
	}

	if (game->rom[i].where != m[i].zno) {
	    warn_rom(game->rom+i, "ought to be in %s, is in %s",
		 zname[game->rom[i].where],
		 zname[m[i].zno]);
	}
    }

    /* analyze result: files */
    for (i=0; i<zip[0].nrom; i++) {
	needed = 0;
	for (j=0; j<game->nrom; j++)
	    if (m[j].zno == 0 && m[j].fno == i) {
		needed = 1;
		break;
	    }

	if (!needed)
	    warn_file(zip[0].rom+i, "not used");
    }
	    

    /* XXX: free zip structures */
}



static int
match(struct game *game, struct zip *zip, int zno, struct match *m)
{
    int i, j;
    enum state st;
    
    for (i=0; i<game->nrom; i++) {
	for (j=0; j<zip->nrom; j++) {
	    st = romcmp(zip->rom+j, game->rom+i);
	    if ((st == ROM_LONG
		 && (makencrc(zip->name, zip->rom[j].name, game->rom[i].size)
		     == game->rom[i].crc)))
		st = ROM_LONGOK;
	    
	    if (st != ROM_UNKNOWN)
		add_match(m+i, game->rom[i].where, zno, j, st);
	}
    }
}



static int
add_match(struct match *m, enum where where, int zno, int fno, enum state st)
{
    struct match *p, *q;

    p = (struct match *)xmalloc(sizeof(struct match));
    p->where = where;
    p->zno = zno;
    p->fno = fno;
    p->quality = st;

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
warn_rom(struct rom *r, char *fmt, ...)
{
    va_list va;

    printf("rom %-12s size %d crc %0.8x: ",
	   r->name, r->size, r->crc);
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}



void
warn_file(struct rom *r, char *fmt, ...)
{
    va_list va;

    printf("file %-12s  size %d crc %0.8x: ",
	    r->name, r->size, r->crc);
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}
