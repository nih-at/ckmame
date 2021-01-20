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

#include "Dir.h"
#include "util.h"

int
images_find(const Images *images, const std::string &name) {
    if (images == NULL) {
        return -1;
    }

    auto full_name = name + ".chd";

    for (size_t i = 0; i < images->disks.size(); i++) {
	if (std::filesystem::path(images->disks[i]->name).filename() == full_name) {
            return i;
        }
    }

    return -1;
}


std::string
images_name(const Images *im, int i) {
    auto disk = im->disks[i];

    return disk ? disk->name : "";
}

ImagesPtr Images::from_directory(const std::string &directory, bool check_integrity) {
    auto images = std::make_shared<Images>();

    auto dirname = std::string(get_directory()) + "/" + directory;

    try {
	 Dir dir(dirname, false);
	 std::filesystem::path filepath;

	 while ((filepath = dir.next()) != "") {
	     if (name_type(filepath) == NAME_CHD) {
		 images->disks.push_back(Disk::from_file(filepath, check_integrity ? DISK_FL_CHECK_INTEGRITY : 0));                                                                                                                                                           }
	 }
    }
    catch (...) {
	return images;
    }

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
