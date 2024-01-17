/*
check_for_file_in_archive.cc -- check if archive contains file, doing minimal work
Copyright (C) 2005-2022 Dieter Baron and Thomas Klausner

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

#include "check_util.h"
#include "Exception.h"
#include "find.h"

class ArchiveFileProxy {
  public:
    ArchiveFileProxy(filetype_t filetype, const std::string& archive_name, std::string file_name, int hashtypes);

    size_t index;

    ArchivePtr archive() { ensure_archive(); return a; }
    size_t size(size_t detector_id);
    const Hashes& hashes(size_t detector_id);

  private:
    filetype_t filetype;
    std::string full_name;
    std::string file_name;
    int hashtypes;

    ArchivePtr a;
    ArchiveContentsPtr contents;
    File* file;

    void ensure_archive();

  private:
    void find_file();
};

ArchiveFileProxy::ArchiveFileProxy(filetype_t filetype, const std::string& archive_name, std::string file_name, int hashtypes) : filetype(filetype), file_name(std::move(file_name)), hashtypes(hashtypes) {
    full_name = findfile(filetype, archive_name);
    if (full_name.empty()) {
        throw Exception();
    }

    contents = ArchiveContents::by_name(filetype, archive_name);
    if (!contents) {
        ensure_archive();
    }
    else {
        find_file();
    }
}

void ArchiveFileProxy::find_file() {
    auto maybe_index = contents->file_index_by_name(file_name);
    if (!maybe_index.has_value()) {
        throw Exception();
    }
    index = maybe_index.value();
    file = &contents->files[index];
}


void ArchiveFileProxy::ensure_archive() {
    if (a) {
        return;
    }

    a = Archive::open(full_name, filetype, FILE_ROMSET, 0);
    if (!a) {
        throw Exception();
    }
    contents = a->contents;
    find_file();
}


size_t ArchiveFileProxy::size(size_t detector_id) {
    if (!file->is_size_known(detector_id)) {
        ensure_archive();
        if (!file->is_size_known(detector_id) && detector_id != 0) {
            a->file_ensure_hashes(index, detector_id, hashtypes);
        }
    }
    return file->get_size(detector_id);
}


const Hashes& ArchiveFileProxy::hashes(size_t detector_id) {
    if (!file->has_all_hashes(detector_id, hashtypes)) {
        ensure_archive();
        a->file_ensure_hashes(index, detector_id, hashtypes);
    }
    return file->get_hashes(detector_id);
}

find_result_t check_for_file_in_archive(filetype_t filetype, size_t detector_id, const std::string &name, const FileData *wanted_file, const FileData *candidate, Match *matches) {

    try {
        auto found = false;
        ArchiveFileProxy xxx(filetype, name, candidate->name, wanted_file->hashes.get_types());

        if (filetype == TYPE_ROM) {
            if (xxx.size(0) == candidate->hashes.size) {
                if (xxx.hashes(0).compare(wanted_file->hashes) == Hashes::MATCH) {
                    found = true;
                }
            }
            if (!found && detector_id != 0) {
                if (xxx.size(detector_id) == candidate->hashes.size) {
                    if (xxx.hashes(detector_id).compare(wanted_file->hashes) == Hashes::MATCH) {
                        found = true;
                    }
                }
            }
        }
        else { // TYPE_DISK
            if (xxx.hashes(0).compare(wanted_file->hashes) == Hashes::MATCH) {
                found = true;
            }
        }
        if (found) {
            if (matches != nullptr) {
                matches->archive = xxx.archive();
                matches->index = xxx.index;
            }
            return FIND_EXISTS;
        }

        return FIND_MISSING;
    }
    catch (...) {
        return FIND_MISSING;
    }
}
