/*
  game.c -- create / free game structure
  Copyright (C) 2004-2007 Dieter Baron and Thomas Klausner

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


#include <stdlib.h>

#include "game.h"
#include "xmalloc.h"


game_t *
game_new(void)
{
    game_t *g;
    int i;

    g = xmalloc(sizeof(*g));

    g->id = -1;
    g->name = g->description = NULL;
    
    for (i=0; i<GAME_RS_MAX; i++) {
	g->rs[i].cloneof[0] = g->rs[i].cloneof[1] = NULL;
	g->rs[i].files = array_new(sizeof(file_t));
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
	array_free(g->rs[i].files, file_finalize);
    }
    array_free(g->disks, disk_finalize);
    free(g);
}
