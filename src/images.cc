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
images_find(const Images *images, const char *name) { 
    if (images == NULL) {
        return -1;
    }
    
    char *full_name;
    xasprintf(&full_name, "%s.chd", name);
    
    for (int i = 0; i < images->disks.size(); i++) {
        if (strcmp(mybasename(images->disks[i]->name.c_str()), full_name) == 0) {
            return i;
        }
    }
    
    return -1;
}


const char *
images_name(const Images *im, int i) {
    auto d = im->disks[i];

    return d ? disk_name(d).c_str() : NULL;
}

ImagesPtr Images::from_directory(const std::string &directory, bool check_integrity) {
    dir_t *dir;
    dir_status_t err;
    char b[8192];

    auto dirname = std::string(get_directory()) + "/" + directory;

    if ((dir = dir_open(dirname.c_str(), 0)) == NULL) {
        return NULL;
    }

    auto images = std::make_shared<Images>();

    while ((err = dir_next(dir, b, sizeof(b))) != DIR_EOD) {
        if (err == DIR_ERROR) {
            /* TODO: handle error */
            continue;
        }

        if (name_type(b) == NAME_CHD) {
            images->disks.push_back(Disk::from_file(b, check_integrity ? DISK_FL_CHECK_INTEGRITY : 0));
        }
    }

    dir_close(dir);

    return images;
}


ImagesPtr Images::from_file(const std::string &name) {
    auto disk = Disk::from_file(name, 0);
    
    if (!disk) {
        return NULL;
    }
    
    auto images = std::make_shared<Images>();
    
    images->disks.push_back(disk);
    
    return images;
}
