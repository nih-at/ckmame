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



#include <string.h>

#include "types.h"
#include "romutil.h"



enum state
romcmp(struct rom *r1, struct rom *r2)
{
    /* r1 is important */
    if (strcasecmp(r1->name, r2->name) == 0) {
	if (r1->size == r2->size) {
	    if (r1->crc == r2->crc || r2->crc == 0 || r1->crc == 0)
		return ROM_OK;
	    else
		return ROM_CRCERR;
	}
	else if (r1->size > r2->size)
	    return ROM_LONG;
	else
	    return ROM_SHORT;
    }
    else if (r1->size == r2->size && r1->crc == r2->crc)
	return ROM_NAMERR;
    else
	return ROM_UNKNOWN;
}



void
game_free(struct game *g, int fullp)
{
    int i;

    free(g->name);
    free(g->cloneof[0]);
    free(g->cloneof[1]);
    free(g->sampleof);
    for (i=0; i<g->nrom; i++)
	free(g->rom[i].name);
    for (i=0; i<g->nsample; i++)
	free(g->sample[i].name);
    if (fullp) {
	free(g->rom);
	free(g->sample);
    }

    free(g);
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



void
zip_free(struct zip *zip)
{
    int i;

    if (zip == NULL)
	return;
    
    free(zip->name);
    for (i=0; i<zip->nrom; i++)
	free(zip->rom[i].name);

    if (zip->nrom)
	free(zip->rom);
    free(zip);
}
