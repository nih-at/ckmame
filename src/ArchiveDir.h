#ifndef HAD_ARCHIVE_DIR_H
#define HAD_ARCHIVE_DIR_H

/*
  ArchiveDir.h -- information about files in a directory
  Copyright (C) 1999-2020 Dieter Baron and Thomas Klausner

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

#include <filesystem>

#include "archive.h"

class ArchiveDir : public Archive {
    class ArchiveFile: public Archive::ArchiveFile {
    public:
        ArchiveFile(FILE *f_) : f(f_) { }
        virtual ~ArchiveFile() { close(); }
        
        virtual void close();
        virtual int64_t read(void *, uint64_t);
        virtual const char *strerror();
        
    private:
        FILE *f;
    };

public:
    ArchiveDir(const std::string &name, filetype_t filetype, where_t where, int flags) : Archive(ARCHIVE_DIR, name, filetype, where, flags) { }
    ArchiveDir(ArchiveContentsPtr contents) : Archive(contents) { }
    virtual ~ArchiveDir() { update_cache(); }

protected:
    virtual bool commit_xxx();
    virtual void commit_cleanup();
    virtual bool file_add_empty_xxx(const std::string &filename);
    virtual bool file_copy_xxx(std::optional<uint64_t> index, Archive *source_archive, uint64_t source_index, const std::string &filename, uint64_t start, std::optional<uint64_t> length);
    virtual bool file_delete_xxx(uint64_t index);
    virtual ArchiveFilePtr file_open(uint64_t index);
    virtual bool file_rename_xxx(uint64_t index, const std::string &filename);
    virtual void get_last_update();
    virtual bool read_infos_xxx();
    virtual bool rollback_xxx(); /* never called if commit never fails */

private:
    struct FileInfo {
        std::filesystem::path name;
        std::filesystem::path data_file_name;
        
        bool apply() const;
        bool discard(ArchiveDir *archive) const;
        
        void clear();
    };
    
    class Change {
    public:
        Change() : mtime(0) { }

        // original.name is set if the file has been renamed
        FileInfo original;
        FileInfo destination;
        time_t mtime;
        
        bool is_unchanged() const {
            return original.name.empty() && destination.name.empty();
        }
        bool is_added() const {
            return original.name.empty() && !destination.name.empty();
        }
        bool is_deleted() const {
            return !original.name.empty() && destination.name.empty();
        }
        bool is_renamed() const;
        bool has_new_data() const;
        
        bool apply(ArchiveDir *archive, uint64_t index);
        void rollback(ArchiveDir *archive);
        
        void clear();
    };
    
    std::vector<Change> changes;
    
    Change *get_change(uint64_t index, bool create = false);
    bool ensure_archive_dir();
    bool file_will_exist_after_commit(std::filesystem::path filename);
    int move_original_file_out_of_the_way(uint64_t index);
    std::filesystem::path get_full_name(uint64_t index);
    std::filesystem::path get_original_data(uint64_t index);
    std::filesystem::path make_full_name(const std::filesystem::path &filename);
    std::filesystem::path make_tmp_name(const std::filesystem::path &filename);
};

#endif // HAD_ARCHIVE_DIR_H
