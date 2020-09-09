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

#include "dir.h"
#include "funcs.h"
#include "util.h"
#include "xmalloc.h"

int
images_find(const images_t *images, const char *name) { 
    if (images == NULL) {
        return -1;
    }
    
    char *full_name;
    xasprintf(&full_name, "%s.chd", name);
    
    for (int i = 0; i < images_length(images); i++) {
        if (strcmp(mybasename(disk_name(images_get(images, i))), full_name) == 0) {
            return i;
        }
    }
    
    return -1;
}


const char *
images_name(const images_t *im, int i) {
    disk_t *d;

    d = images_get(im, i);

    return d ? disk_name(d) : NULL;
}


images_t *
images_new(const char *name, int flags) {
    images_t *im;
    dir_t *dir;
    dir_status_t err;
    char b[8192];
    char *dirname;

    xasprintf(&dirname, "%s/%s", get_directory(), name);

    if ((dir = dir_open(dirname, 0)) == NULL) {
        free(dirname);
        return NULL;
    }
    
    im = parray_new();

    while ((err = dir_next(dir, b, sizeof(b))) != DIR_EOD) {
        if (err == DIR_ERROR) {
            /* TODO: handle error */
            continue;
        }

        if (name_type(b) == NAME_CHD) {
            parray_push(im, disk_new(b, flags));
        }
    }

    dir_close(dir);
    free(dirname);

    return im;
}


images_t *
images_new_name(const char *name, int flags) {
    images_t *im;

    im = parray_new_sized(1);

    parray_push(im, disk_new(name, flags));

    return im;
}
