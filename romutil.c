/*
  romutil.c -- miscellaneous utility functions for rom handling
  Copyright (C) 1999 Dieter Baron and Thomas Klaunser

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "romutil.h"
#include "xmalloc.h"



void
rom_add_name(struct rom *r, char *name)
{
    r->altname = realloc(r->altname, (r->naltname+2)*sizeof(char *));

    r->altname[r->naltname] = xstrdup(name);
    r->altname[r->naltname+1] = NULL;

    r->naltname++;

    return;
}

enum state
romcmp(struct rom *r1, struct rom *r2, int merge)
{
    /* XXX: return ROM_UNKNOWN, not ROM_NAMEERR, for samples */
    /* r1 is important */
    /* in match: r1 is from zip, r2 from rom */
    
    if (strcasecmp(r1->name,
		   (merge ? (r2->merge ? r2->merge : r2->name)
		    : r2->name)) == 0) {
	if (r2->size == 0)
	    return ROM_OK;
	if (r1->size == r2->size) {
	    if (r1->crc == r2->crc || r2->crc == 0 || r1->crc == 0)
		return ROM_OK;
	    else if (((r1->crc ^ r2->crc) & 0xffffffff) == 0xffffffff)
		return ROM_BESTBADDUMP;
	    else
		return ROM_CRCERR;
	}
	else if (r1->size > r2->size)
	    return ROM_LONG;
	else
	    return ROM_SHORT;
    }
    else if (r1->size == r2->size && r1->crc == r2->crc && r2->size != 0)
	return ROM_NAMERR;
    else
	return ROM_UNKNOWN;
}



void
game_free(struct game *g, int fullp)
{
    int i, j;

    free(g->name);
    free(g->description);
    free(g->cloneof[0]);
    free(g->cloneof[1]);
    if (g->nclone) {
	for (i=0; i<g->nclone; i++)
	    free(g->clone[i]);
	free(g->clone);
    }
    free(g->sampleof[0]);
    free(g->sampleof[1]);
    if (g->nsclone) {
	for (i=0; i<g->nsclone; i++)
	    free(g->sclone[i]);
	free(g->sclone);
    }
    for (i=0; i<g->nrom; i++) {
	free(g->rom[i].name);
	free(g->rom[i].merge);
	for (j=0; j<g->rom[i].naltname; j++)
	    free(g->rom[i].altname[j]);
	free(g->rom[i].altname);
    }
    for (i=0; i<g->nsample; i++) {
	free(g->sample[i].name);
	free(g->sample[i].merge);
    }
    if (fullp) {
	free(g->rom);
	free(g->sample);
    }
    free(g);
}



void
game_swap_rs(struct game *g)
{
    struct rom *rp;
    char **sp, *s;
    int i;
    
    for (i=0; i<2; i++) {
	s = g->cloneof[i];
	g->cloneof[i] = g->sampleof[i];
	g->sampleof[i] = s;
    }

    i = g->nclone;
    sp = g->clone;
    g->nclone = g->nsclone;
    g->clone = g->sclone;
    g->nsclone = i;
    g->sclone = sp;

    i = g->nrom;
    rp = g->rom;
    g->nrom = g->nsample;
    g->rom = g->sample;
    g->nsample = i;
    g->sample = rp;
}



static void delchecked_r(struct tree *t, int nclone, char **clone);

char **
delchecked(struct tree *t, int nclone, char **clone)
{
    char **need;

    need = (char **)xmalloc(sizeof(char *)*nclone);
    memcpy(need, clone, sizeof(char *)*nclone);

    delchecked_r(t->child, nclone, need);

    return need;
}



static void
delchecked_r(struct tree *t, int nclone, char **clone)
{
    int i, cmp;
    
    for (; t; t=t->next) {
	for (i=0; i<nclone; i++) {
	    if (clone[i]) {
		cmp = strcmp(clone[i], t->name);
		if (cmp == 0) {
		    clone[i] = NULL;
		    break;
		}
		else if (cmp > 0)
		    break;
	    }
	}
	if (t->child)
	    delchecked_r(t->child, nclone, clone);
    }
}

