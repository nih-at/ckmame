#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "types.h"
#include "dbl.h"
#include "error.h"
#include "util.h"
#include "romutil.h"
#include "xmalloc.h"

extern char *prg;

static int fix_file(struct rom *rom, struct match *m, struct zip **zip);
static int fix_add_garbage(struct zip *zip, int idx);

static struct zf *zf_garbage;



int
fix_game(struct game *g, struct zip **zip, struct match *m)
{
    int i;

    zf_garbage = NULL;

    for (i=0; i<g->nrom; i++) {
	if (m[i].quality < ROM_NAMERR)
	    continue;
	
	if (g->rom[i].where == ROM_INZIP && m[i].zno != ROM_INZIP) {
	    fix_file(g->rom+i, m+i, zip);
	    zip[m[i].zno]->rom[m[i].fno].state = ROM_NAMERR;
	}
	if (m[i].zno == ROM_INZIP && m[i].quality < ROM_BESTBADDUMP) {
	    fix_file(g->rom+i, m+i, zip);
	}
    }

    for (i=0; i<zip[0]->nrom; i++) {
	if ((zip[0]->rom[i].state == ROM_UNKNOWN
	    || (zip[0]->rom[i].state < ROM_NAMERR
		&& zip[0]->rom[i].where != 0))
	    && (output_options & WARN_UNKNOWN)) {
	    printf("%s: mv/rm unknown file %s\n",
		   zip[0]->name, zip[0]->rom[i].name);
#ifdef CHANGE_ZIP
	    if (keep_unknown)
		fix_add_garbage(*(zip[0]), i);
	    zip_delete(zip[0]->zf, i);
#endif
	}
	else if ((zip[0]->rom[i].state < ROM_TAKEN)
		 && (output_options & WARN_NOT_USED)) {
	    printf("%s: mv/rm unused file %s\n",
		   zip[0]->name, zip[0]->rom[i].name);
#ifdef CHANGE_ZIP
	    if (keep_unused)
		fix_add_garbage(*(zip[0]), i);
	    zip_delete(zip[0]->zf, i);
#endif
	}
    }

    if (zf_garbage)
	zip_close(zf_garbage);

    return 0;
}



static int
fix_file(struct rom *rom, struct match *m, struct zip **zip)
{
    if (m->zno != 0) {
	if (m->quality == ROM_LONGOK) {
#ifdef CHANGE_ZIP
	    zip_add_zip(zip[0]->zf, rom->name,
			zip[m->zno]->zf, m->fno, m->offset, rom->size);
	    if (keep_long)
		fix_add_garbage(*(zip[m->zno]), m->fno);
#endif
	    printf("%s: add `%s/%s' as %s, shrinking to %d/%ld\n",
		   zip[0]->name,
		   zip[m->zno]->name, zip[m->zno]->rom[m->fno].name,
		   rom->name, m->offset, rom->size);
	}
	else {
#ifdef CHANGE_ZIP
	    zip_add_zip(zip[0]->zf, rom->name,
			zip[m->zno]->zf, m->fno, 0, 0);
#endif
	    printf("%s: add `%s/%s' as %s\n",
		   zip[0]->name,
		   zip[m->zno]->name, zip[m->zno]->rom[m->fno].name,
		   rom->name);
	}
    }
    else {
	switch (m->quality) {
	case ROM_NAMERR:
#ifdef CHANGE_ZIP
	    zip_rename(zip[0]->zf, m->fno, rom->name);
#endif
	    printf("%s: rename `%s' to %s\n",
		   zip[0]->name,
		   zip[0]->rom[m->fno].name,
		   rom->name);
	    break;

	case ROM_LONGOK:
#ifdef CHANGE_ZIP
	    zip_replace_zip(zip[0]->zf, m->fno, rom->name,
			    zip[0]->zf, m->fno, m->offset, rom->size);
	    if (keep_long)
		fix_add_garbage(*(zip[m->zno]), m->fno);
#endif
	    printf("%s: shrink `%s' as %s to %d/%ld\n",
		   zip[0]->name,
		   zip[0]->rom[m->fno].name,
		   rom->name, m->offset, rom->size);
	    break;

	default:
	}
    }

    return 0;
}



static int
fix_add_garbage(struct zip *zip, int idx)
{
#ifdef CHANGE_ZIP
    char *name;

    if (zf_garbage == NULL) {
	name = mkgarbage_name(zip->name);
	zf_garbage = zip_open(name, ZIP_CREATE);
	free(name);
    }
    if (zf_garbage)
	zip_add_zip(zf_garbage, NULL, zip->zf, idx, 0, 0);
#endif

    return 0;
}



static char *
mkgarbage_name(char *name)
{
    char *s;

    /* XXX: requires roms/ prefix; requires major rewrite */

    s = (char *)xmalloc(strlen(name+5)+strlen("garbage/")+1);

    sprintf(s, "garbage/%s", name+5);

    return s;
}
