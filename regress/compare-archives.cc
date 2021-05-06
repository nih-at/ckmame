/*
  compare-archive.cc -- compare two archives with libarchive
  Copyright (C) 2021 Dieter Baron and Thomas Klausner

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

#include "config.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include <map>
#include <string>

#ifndef HAVE_LIBARCHIVE

int main(int argc, char *argv[]) {
    fprintf(stderr, "%s: missing libarchive support\n", argv[0]);
    exit(1);
}

#else

#include <archive.h>
#include <archive_entry.h>
#include <zlib.h>

const char *prg;

class FileData {
public:
    uint64_t size;
    uint32_t crc;
};

std::map<std::string, FileData> list_archive(const std::string &name) {
    std::map<std::string, FileData> files;
    
    auto archive = archive_read_new();
    if (archive == NULL) {
        fprintf(stderr, "%s: out of memory\n", prg);
        exit(2);
    }
    
    archive_read_support_filter_all(archive);
    archive_read_support_format_all(archive);
    
    if (archive_read_open_filename(archive, name.c_str(), 10240) < 0) {
        fprintf(stderr, "%s: error opening archive '%s': %s\n", prg, name.c_str(), archive_error_string(archive));
        exit(2);
    }

    int ret;
    
    struct archive_entry *entry;

    while ((ret = archive_read_next_header(archive, &entry)) == ARCHIVE_OK) {
        FileData file;
        
        std::string name = archive_entry_pathname_utf8(entry);
        file.size = static_cast<uint64_t>(archive_entry_size(entry));

        uint8_t buffer[BUFSIZ];
        
        file.crc = static_cast<uint32_t>(crc32(0L, NULL, 0));
        while (auto n = archive_read_data(archive, buffer, sizeof(buffer)) > 0) {
            file.crc = static_cast<uint32_t>(crc32(file.crc, buffer, n));
        }
        
        files[name] = file;
    }
    
    archive_read_free(archive);
    
    return files;
}

bool compare(const std::map<std::string, FileData> &a, const std::map<std::string, FileData> &b, const std::string &a_name, const std::string &b_name, bool verbose) {
    auto changed = false;
    
    auto it_a = a.begin();
    auto it_b = b.begin();
    
    while (it_a != a.end() || it_b != b.end()) {
        auto print_a = false;
        auto print_b = false;
        if (it_a != a.end() && it_b != b.end()) {
            if (it_a->first < it_b->first) {
                print_a = true;
            }
            else if (it_a->first > it_b->first) {
                print_b = true;
            }
            else {
                it_a++;
                it_b++;
                continue;
            }
        }
        else if (it_a != a.end()) {
            print_a = true;
        }
        else {
            print_b = true;
        }
        
        if (!changed) {
            if (verbose) {
                printf("--- %s\n", a_name.c_str());
                printf("+++ %s\n", b_name.c_str());
            }

            changed = true;
        }
        
        if (print_a) {
            if (verbose) {
                printf("- file '%s', size %" PRIu64 ", crc %08x\n", it_a->first.c_str(), it_a->second.size, it_a->second.crc);
            }
            it_a++;
        }
	if (print_b) {
            if (verbose) {
                printf("+ file '%s', size %" PRIu64 ", crc %08x\n", it_b->first.c_str(), it_b->second.size, it_b->second.crc);
            }
            it_b++;
        }
    }
    
    return !changed;
}

void usage() {
    fprintf(stderr, "Usage: %s archive-a archive-b\n", prg);
    exit(2);
}

int main(int argc, char *argv[]) {
    prg = argv[0];
    
    if (argc < 3) {
        usage();
    }
    
    auto verbose = false;
    
    if (std::string(argv[1]) == "-v") {
        if (argc < 4) {
            usage();
        }
        argv++;

        verbose = true;
    }
    
    auto a = list_archive(argv[1]);
    auto b = list_archive(argv[2]);
    
    if (!compare(a, b, argv[1], argv[2], verbose)) {
        exit(1);
    }
    
    exit(0);
}


#endif
