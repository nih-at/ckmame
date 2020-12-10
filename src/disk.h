#ifndef HAD_DISK_H
#define HAD_DISK_H

/*
  disk.h -- information about one disk
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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


#include "hashes.h"
#include "types.h"

#include <memory>

class Disk;

typedef std::shared_ptr<Disk> DiskPtr;

class Disk {
public:
    uint64_t id;
    int refcount;
    std::string name;
    std::string merge;
    Hashes hashes;
    status_t status;
    where_t where;
    
    Disk() : status(STATUS_OK), where(FILE_INGAME) { }
    
    static DiskPtr from_file(const std::string &name, int flags);
    
    static DiskPtr by_id(uint64_t id);
    static DiskPtr by_name(const std::string &name);
    
    const std::string &merged_name() const { return merge.empty() ? name : merge; }
    bool compare_merge(const Disk &other) const;
    bool compare_merge_hashes(const Disk &other) const;
    bool compare_hashes(const Disk &other) const;
    bool is_mergeable(const Disk &other) const;

    static uint64_t next_id;
    static std::unordered_map<std::string, std::weak_ptr<Disk>> disk_by_name;
    static std::unordered_map<uint64_t, DiskPtr> disk_by_id;
};

#define DISK_FL_CHECK_INTEGRITY 0x2
#define DISK_FL_QUIET 0x4


#define disk_hashes(d) (&(d)->hashes)
#define disk_status(d) ((d)->status)
#define disk_where(d) ((d)->where)
#define disk_merge(d) ((d)->merge)
#define disk_merged_name(d) ((d)->merged_name())
#define disk_name(d) ((d)->name)
#define disk_id(d) ((d)->id)

#define disk_by_id(i) (Disk::by_id(i))

#define disk_compare_merge(a, b) ((a)->compare_merge(*(b)))
#define disk_compare_merge_hashes(a, b) ((a)->compare_merge_hashes(*(b)))
#define disk_compare_hashes(a, b) ((a)->compare_hashes(*(b)))
#define disk_mergeable(a, b) ((a)->is_mergeable(*(b)))

#endif /* disk.h */
