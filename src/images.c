/*
  images.c -- array of disk images
  Copyright (C) 2006-2014 Dieter Baron and Thomas Klausner

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


#include "images.h"
#include "funcs.h"


const char *
images_name(const images_t *im, int i)
{
    disk_t *d;
    
    d = images_get(im, i);

    return d ? disk_name(d) : NULL;
}


images_t *
images_new(const game_t *g, int flags)
{
    images_t *im;
    char *fname;
    int i;

    if (game_num_disks(g) == 0)
	return NULL;

    im = parray_new_sized(game_num_disks(g));

    for (i=0; i<game_num_disks(g); i++) {
	fname = findfile(disk_name(game_disk(g, i)), TYPE_DISK);
	if (fname == NULL && disk_merge(game_disk(g, i)) != NULL)
	    fname = findfile(disk_merge(game_disk(g, i)), TYPE_DISK);
	if (fname == NULL)
	    parray_push(im, NULL);
	else {
	    parray_push(im, disk_new(fname, flags));
	    free(fname);
	}
    }

    return im;
}


images_t *
images_new_name(const char *name, int flags)
{
    images_t *im;

    im = parray_new_sized(1);

    parray_push(im, disk_new(name, flags));

    return im;
}
