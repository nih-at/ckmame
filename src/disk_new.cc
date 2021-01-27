/*
  disk_new.c -- create / free disk structure from image
  Copyright (C) 2004-2014 Dieter Baron and Thomas Klausner

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


#include "chd.h"
#include "disk.h"
#include "error.h"
#include "Exception.h"
#include "globals.h"
#include "memdb.h"


DiskPtr Disk::from_file(const std::string &name, int flags) {
    if (name.empty()) {
	return NULL;
    }

    auto disk = by_name(name);
    
    if (disk) {
        return disk;
    }

    seterrinfo(name);

    disk = std::make_shared<Disk>();
    disk->name = name;

    try {
	auto chd = std::make_unique<Chd>(name);

	if (chd->flags & CHD_FLAG_HAS_PARENT) {
            throw Exception("parent image required");
	}


	if (flags & DISK_FL_CHECK_INTEGRITY) {
	    disk->hashes.types = db->hashtypes(TYPE_DISK);

            chd->get_hashes(&disk->hashes);

	    if (disk->hashes.has_type(Hashes::TYPE_MD5)) {
		if (!disk->hashes.verify(Hashes::TYPE_MD5, chd->md5)) {
                    throw Exception("md5 mismatch");
		}
	    }

	    if (chd->version > 2 && disk->hashes.has_type(Hashes::TYPE_SHA1)) {
		if (!disk->hashes.verify(Hashes::TYPE_SHA1, chd->sha1)) {
                    throw ("sha1 mismatch");
		}
	    }
	}

	if (chd->version < 4 && !disk->hashes.has_type(Hashes::TYPE_MD5)) {
	    disk->hashes.set(Hashes::TYPE_MD5, chd->md5);
	}
	if (chd->version > 2 && !disk->hashes.has_type(Hashes::TYPE_SHA1)) {
	    disk->hashes.set(Hashes::TYPE_SHA1, chd->sha1);
	}
    }
    catch (Exception &e) {
	myerror(ERRFILE, "error opening disk: %s", e.what());
        disk->status = STATUS_BADDUMP;
    }

    disk->id = ++next_id;
    disk_by_id[disk->id] = disk;
    disk_by_name[name] = disk;

    return disk;
}
