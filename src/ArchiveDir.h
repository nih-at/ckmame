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
#include <unordered_set>

#include "Archive.h"
#include "SharedFile.h"

class ArchiveDir : public Archive {
public:
    ArchiveDir(const std::string &name, filetype_t filetype, where_t where, int flags) : Archive(ARCHIVE_DIR, name, filetype, where, flags) { }
    ArchiveDir(ArchiveContentsPtr contents) : Archive(contents) { }
    virtual ~ArchiveDir() { update_cache(); }

protected:
    virtual bool commit_xxx();
    virtual void commit_cleanup();
    virtual void get_last_update();
    virtual bool read_infos_xxx();
    virtual ZipSourcePtr get_source(uint64_t index, uint64_t start, std::optional<uint64_t> length);
    virtual bool have_direct_file_access() const { return true; }
    virtual std::string get_full_filename(uint64_t index);
    virtual std::string get_original_filename(uint64_t index);

private:
    class Commit {
    public:
        Commit(ArchiveDir *archive);
        
        void delete_file(const std::filesystem::path &file);
        void rename_file(const std::filesystem::path &source, const std::filesystem::path &destination);

        void undo();
        void done();

    private:
        class Operation {
        public:
            enum Type {
                DELETE,
                RENAME
            };

            Operation(const std::filesystem::path &old_name_, const std::filesystem::path &new_name_) : type(RENAME), old_name(old_name_), new_name(new_name_) { }
            Operation(const std::filesystem::path &name) : type(DELETE), new_name(name) { }
            Type type;
            std::filesystem::path old_name;
            std::filesystem::path new_name;
            
            void execute();
        };

        ArchiveDir *archive;
        
        std::filesystem::path deleted_directory;
        std::vector<Operation> undos;
        std::unordered_set<std::string> cleanup_directories;
        std::unordered_map<std::string, std::filesystem::path> renamed_files;

        std::filesystem::path get_filename(const std::filesystem::path &filename);
        void ensure_file_doesnt_exist(const std::filesystem::path &file);
        void ensure_parent_directory(const std::filesystem::path &file);
        void rename(const std::filesystem::path &source, const std::filesystem::path &destination);
    };
    
    bool ensure_archive_dir();

    void copy_source(ZipSource *source, const std::filesystem::path &destination);

    time_t get_mtime(const std::string &file);
};

#endif // HAD_ARCHIVE_DIR_H
