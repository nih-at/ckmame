#ifndef _HAD_ARCHIVE_LIBARCHIVE_H
#define _HAD_ARCHIVE_LIBARCHIVE_H

/*
  ArchiveZip.h -- archive via libarchive
  Copyright (C) 1999-2021 Dieter Baron and Thomas Klausner

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

#include "ArchiveZip.h"

#include <archive.h>

class ArchiveLibarchive : public ArchiveZip {
    class ArchiveFile: public Archive::ArchiveFile {
    public:
        ArchiveFile(ArchiveLibarchive *archive_, struct archive *la_) : archive(archive_), la(la_) { }
        virtual ~ArchiveFile() { close(); }
        
        virtual void close();
        virtual int64_t read(void *, uint64_t);
        virtual const char *strerror();
        
    private:
        ArchiveLibarchive *archive;
        struct archive *la;
    };
    
public:
    ArchiveLibarchive(const std::string &name, filetype_t filetype, where_t where, int flags) : ArchiveZip(name, filetype, where, flags | ARCHIVE_FL_RDONLY), la(NULL), current_index(0), have_open_file(false) { contents->archive_type = ARCHIVE_LIBARCHIVE; }
    ArchiveLibarchive(ArchiveContentsPtr contents) : ArchiveZip(contents), la(NULL), current_index(0), have_open_file(false) { }

    virtual ~ArchiveLibarchive() { close(); }

    virtual bool check();
    virtual bool close_xxx();
    virtual bool commit_xxx();
    virtual void commit_cleanup();
    virtual ArchiveFilePtr file_open(uint64_t index);
    virtual bool read_infos_xxx();
    virtual bool rollback_xxx(); /* never called if commit never fails */

protected:
    virtual zip_source_t *get_source(zip_t *destination_archive, uint64_t index, uint64_t start, std::optional<uint64_t> length);

private:
    class Source {
    public:
        Source(ArchiveLibarchive *archive_, uint64_t index_, uint64_t start_, uint64_t length_): archive(archive_), index(index_), start(start_), length(length_) { zip_error_init(&error); }
        
        static zip_int64_t callback_c(void *userdata, void *data, zip_uint64_t len, zip_source_cmd_t cmd);
        zip_int64_t callback(void *data, zip_uint64_t len, zip_source_cmd_t cmd);

        zip_source_t *get_source(zip_t *za);
        
        ArchiveLibarchive *archive;
        uint64_t index;
        uint64_t start;
        uint64_t length;

        ArchiveFilePtr file;
        zip_error_t error;
    };
    
    struct archive *la;
    uint64_t current_index;
    bool have_open_file;
    
    bool ensure_la();
    
};


#endif // _HAD_ARCHIVE_LIBARCHIVE_H
