#ifndef HAD_ARCHIVE_H
#define HAD_ARCHIVE_H

/*
  archive.h -- information about an archive
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


#include <sys/stat.h>
#include <sys/types.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "dbh.h"
#include "file.h"

class Archive;
class ArchiveFile;
class ArchiveContents;

typedef std::shared_ptr<Archive> ArchivePtr;
typedef std::shared_ptr<ArchiveContents> ArchiveContentsPtr;

#define ARCHIVE_FL_CREATE 0x00100
#define ARCHIVE_FL_CHECK_INTEGRITY 0x00200
#define ARCHIVE_FL_QUIET 0x00400
#define ARCHIVE_FL_NOCACHE 0x00800
#define ARCHIVE_FL_RDONLY 0x01000
#define ARCHIVE_FL_TOP_LEVEL_ONLY 0x02000
#define ARCHIVE_FL_KEEP_EMPTY 0x04000

#define ARCHIVE_FL_HASHTYPES_MASK 0x000ff
#define ARCHIVE_FL_MASK 0x0ff00

/* internal */
extern int _archive_global_flags;

void archive_global_flags(int fl, bool setp);

#define ARCHIVE_IS_INDEXED(a) (((a)->flags & ARCHIVE_FL_NOCACHE) == 0 && IS_EXTERNAL(archive_where(a)))

enum ArchiveType {
    ARCHIVE_ZIP,
    ARCHIVE_DIR
};

class ArchiveContents {
public:
    ArchiveContents(ArchiveType type, const std::string &name, filetype_t filetype, where_t where, int flags);

    uint64_t id;
    std::string name;
    std::vector<File> files;
    filetype_t filetype;
    where_t where;

    std::shared_ptr<DB> cache_db;
    int cache_id;
    int flags;
    time_t mtime;
    uint64_t size;
    
    ArchiveType archive_type;
    std::weak_ptr<Archive> open_archive;
    
  
    bool read_infos_from_cachedb(std::vector<File> *files);
    int is_cache_up_to_date();

    static void enter_in_maps(ArchiveContentsPtr contents);
    static ArchiveContentsPtr by_id(uint64_t id);
    static ArchiveContentsPtr by_name(const std::string &name);

private:
    static uint64_t next_id;
    static std::unordered_map<std::string, std::weak_ptr<ArchiveContents>> archive_by_name;
    static std::unordered_map<uint64_t, ArchiveContentsPtr> archive_by_id;

};


class Archive {
public:
    class ArchiveFile {
    public:
        virtual ~ArchiveFile() { }
        
        virtual void close() = 0;
        virtual int64_t read(void *, uint64_t) = 0;
        virtual const char *strerror() = 0;
    };
    typedef std::shared_ptr<ArchiveFile> ArchiveFilePtr;

    static ArchivePtr by_id(uint64_t id);
    
    static ArchivePtr open(const std::string &name, filetype_t filetype, where_t where, int flags);
    static ArchivePtr open_toplevel(const std::string &name, filetype_t filetype, where_t where, int flags);
    
    static ArchivePtr open(ArchiveContentsPtr contents);
        
    static int64_t file_read_c(void *fp, void *data, uint64_t length);

    static int register_cache_directory(const std::string &name);
    
    Archive(ArchiveContentsPtr contents_);
    virtual ~Archive() { /*printf("# destroying %s\n", name.c_str());*/ }

    int close();
    bool commit();
    void ensure_valid_archive();
    bool file_add_empty(const std::string &filename);
    int file_compare_hashes(uint64_t idx, const Hashes *h);
    bool file_compute_hashes(uint64_t idx, int hashtypes);
    bool file_copy(Archive *source_archive, uint64_t source_index, const std::string &filename);
    bool file_copy_or_move(Archive *source_archive, uint64_t source_index, const std::string &filename, bool copy);
    bool file_copy_part(Archive *source_archive, uint64_t source_index, const std::string &filename, uint64_t start, std::optional<uint64_t> length, const File *f);
    bool file_delete(uint64_t index);
    std::optional<size_t> file_find_offset(size_t idx, size_t size, const Hashes *h);
    std::optional<size_t> file_index_by_hashes(const Hashes *h) const;
    std::optional<size_t> file_index_by_name(const std::string &name) const;
    std::optional<size_t> file_index(const File *file) const;
    void file_match_detector(uint64_t idx);
    bool file_move(Archive *source_archive, uint64_t source_index, const std::string &filename);
    bool file_rename(uint64_t index, const std::string &filename);
    bool file_rename_to_unique(uint64_t index);
    std::string make_unique_name(const std::string &filename);
    bool read_infos();
    void refresh();
    bool rollback();
    bool is_empty() const;
    bool is_writable() const { return (contents->flags & ARCHIVE_FL_RDONLY) == 0; }
    bool is_indexed() const { return (contents->flags & ARCHIVE_FL_NOCACHE) == 0 && IS_EXTERNAL(where);
}
    virtual bool check() { return true; } // This is done as part of the constructor, remove?
    virtual bool close_xxx() { return true; }
    virtual bool commit_xxx() = 0;
    virtual void commit_cleanup() = 0;
    virtual bool file_add_empty_xxx(const std::string &filename) = 0;
    virtual bool file_copy_xxx(std::optional<uint64_t> index, Archive *source_archive, uint64_t source_index, const std::string &filename, uint64_t start, std::optional<uint64_t> length) = 0;
    virtual bool file_delete_xxx(uint64_t index) = 0;
    virtual ArchiveFilePtr file_open(uint64_t index) = 0;
    virtual bool file_rename_xxx(uint64_t index, const std::string &filename) = 0;
    virtual void get_last_update() = 0;
    virtual bool read_infos_xxx() = 0;
    virtual bool rollback_xxx() = 0; /* never called if commit never fails */
    
    ArchiveContentsPtr contents;
    std::vector<File> &files;
    std::string &name;
    const filetype_t filetype;
    const where_t where;
    
    bool cache_changed;
    bool modified;
    
protected:
    Archive(ArchiveType type, const std::string &name, filetype_t filetype, where_t where, int flags);
    void update_cache();

    void add_file(const std::string &filename, const File *file);
    bool get_hashes(ArchiveFile *f, size_t len, Hashes *h);
    void merge_files(const std::vector<File> &files_cache);
    
};

#endif /* archive.h */
