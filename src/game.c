/*
  $NiH$

  game.c -- create / free game structure
  Copyright (C) 2004, 2005 Dieter Baron and Thomas Klausner

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

#include "game.h"
#include "xmalloc.h"



game_t *
game_new(void)
{
    game_t *g;
    int i;

    g = xmalloc(sizeof(*g));
    
    g->name = g->description = NULL;
    
    for (i=0; i<GAME_RS_MAX; i++) {
	g->rs[i].cloneof[0] = g->rs[i].cloneof[1] = NULL;
	g->rs[i].clones = parray_new();
	g->rs[i].files = array_new(sizeof(rom_t));
    }
    
    g->disks = array_new(sizeof(disk_t));

    return g;
}



void
game_free(game_t *g)
{
    int i;

    if (g == NULL)
	return;

    free(g->name);
    free(g->description);

    for (i=0; i<GAME_RS_MAX; i++) {
	free(g->rs[i].cloneof[0]);
	free(g->rs[i].cloneof[1]);
	parray_free(g->rs[i].clones, free);
	array_free(g->rs[i].files, rom_finalize);
    }
    array_free(g->disks, disk_finalize);
    free(g);
}
