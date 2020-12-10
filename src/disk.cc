/*
  disk.c -- initialize / finalize disk structure
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


#include "disk.h"

uint64_t Disk::next_id;
std::unordered_map<std::string, std::weak_ptr<Disk>> Disk::disk_by_name;
std::unordered_map<uint64_t, DiskPtr> Disk::disk_by_id;


DiskPtr Disk::by_id(uint64_t id) {
    auto it = disk_by_id.find(id);
    
    if (it == disk_by_id.end()) {
        return NULL;
    }
    return it->second;
}


DiskPtr Disk::by_name(const std::string &name) {
    auto it = disk_by_name.find(name);
    
    if (it == disk_by_name.end()) {
        return NULL;
    }
    
    return it->second.lock();
}


bool Disk::compare_merge(const Disk &other) const {
    return merged_name() == other.name;
}


bool Disk::compare_merge_hashes(const Disk &other) const {
    return compare_merge(other) && compare_hashes(other);
}

bool Disk::compare_hashes(const Disk &other) const {
    return hashes.compare(other.hashes) != Hashes::MISMATCH;
}


bool Disk::is_mergeable(const Disk &other) const {
    /* name must match (merged) name */
    if (!compare_merge(other)) {
	return false;
    }
    /* both can be bad dumps */
    if (hashes.empty() && other.hashes.empty()) {
	return true;
    }
    /* or the hashes must match */
    if (!hashes.empty() && !other.hashes.empty() && compare_hashes(other)) {
	return true;
    }
    return false;
}
