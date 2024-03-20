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

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <zip.h>

#include "CkmameDB.h"
#include "Detector.h"
#include "File.h"
#include "types.h"
#include "zip_util.h"

class Archive;
class ArchiveContents;

typedef std::shared_ptr<Archive> ArchivePtr;
typedef std::shared_ptr<ArchiveContents> ArchiveContentsPtr;

#define ARCHIVE_FL_CREATE 0x00100
#define ARCHIVE_FL_RDONLY 0x01000
#define ARCHIVE_FL_TOP_LEVEL_ONLY 0x02000

#define ARCHIVE_FL_HASHTYPES_MASK 0x000ff
#define ARCHIVE_FL_MASK 0x0ff00


enum ArchiveType {
    ARCHIVE_ZIP,
    ARCHIVE_LIBARCHIVE,
    ARCHIVE_DIR,
    ARCHIVE_IMAGES
};

class ArchiveContents {
public:
    ArchiveContents(ArchiveType type, std::string name, filetype_t filetype, where_t where, int flagsk, std::string filename_extension = "");

    uint64_t id;
    std::string name;
    std::vector<File> files;
    filetype_t filetype;
    where_t where;

    CkmameDBPtr cache_db;
    int cache_id;
    int flags;
    time_t mtime;
    uint64_t size;
    
    ArchiveType archive_type;
    std::weak_ptr<Archive> open_archive;
    std::string filename_extension;
  
    [[nodiscard]] std::optional<size_t> file_index_by_name(const std::string &name) const;
    bool has_all_detector_hashes(const std::unordered_map<size_t, DetectorPtr> &detectors);
    
    bool read_infos_from_cachedb(std::vector<File> *cached_files);
    [[nodiscard]] int is_cache_up_to_date() const;

    static void enter_in_maps(const ArchiveContentsPtr& contents);
    static ArchiveContentsPtr by_name(filetype_t filetype, const std::string &name);
    static void clear_cache();

    class TypeAndName {
    public:
        TypeAndName(filetype_t filetype_, std::string name_) : filetype(filetype_), name(std::move(name_)) { }
        
        filetype_t filetype;
        std::string name;
        
        bool operator==(const TypeAndName &other) const { return filetype == other.filetype && name == other.name; }
    };
    
private:
    static std::unordered_map<TypeAndName, ArchiveContentsPtr> archive_by_name;
};

namespace std {
template <>
struct hash<ArchiveContents::TypeAndName> {
    std::size_t operator()(const ArchiveContents::TypeAndName &k) const {
        return std::hash<int>()(k.filetype) ^ std::hash<std::string>()(k.name);
    }
};
}

class Archive {
public:
    class Change {
    public:
        enum Status {
            ADDED,
            DELETED,
            EXISTS
        };
        
        Change(): status(EXISTS) { }
        
        Status status;
        std::string original_name;
        std::string source_name;
        ZipSourcePtr source;
        std::string file;
    };

    static ArchivePtr open(const std::string &name, filetype_t filetype, where_t where, int flags);
    static ArchivePtr open_toplevel(const std::string &name, filetype_t filetype, where_t where, int flags);
    
    static ArchivePtr open(const ArchiveContentsPtr& contents, int flags = 0);

    static bool read_only_mode;

    explicit Archive(ArchiveContentsPtr contents_);
    virtual ~Archive() = default;

    int close();
    bool commit();
    bool compare_size_hashes(size_t index, size_t detector_id, const FileData *rom);
    bool compute_detector_hashes(const std::unordered_map<size_t, DetectorPtr> &detectors);
    void ensure_valid_archive();
    bool file_add_empty(const std::string &filename);
    int file_compare_hashes(uint64_t idx, const Hashes *h);
    virtual bool file_ensure_hashes(uint64_t idx, int hashtypes) { return file_ensure_hashes(idx, 0, hashtypes); }
    bool file_ensure_hashes(uint64_t index, size_t detector_id, int hashtypes);
    bool file_copy(Archive *source_archive, uint64_t source_index, const std::string &filename);
    bool file_copy_or_move(Archive *source_archive, uint64_t source_index, const std::string &filename, bool copy);
    bool file_copy_part(Archive *source_archive, uint64_t source_index, const std::string &filename, uint64_t start, std::optional<uint64_t> length, const Hashes *hashes);
    bool file_delete(uint64_t index);
    std::optional<size_t> file_find_offset(size_t idx, size_t size, const Hashes *h);
    [[nodiscard]] std::optional<size_t> file_index_by_name(const std::string &name) const;
    std::optional<size_t> file_index(const FileData *file) const;
    bool file_move(Archive *source_archive, uint64_t source_index, const std::string &filename);
    bool file_rename(uint64_t index, const std::string &filename);
    bool file_rename_to_unique(uint64_t index);
    std::string make_unique_name_in_archive(const std::string &filename);
    bool read_infos();
    bool rollback();
    [[nodiscard]] bool is_empty() const;
    [[nodiscard]] bool is_file_deleted(uint64_t index) const { return changes[index].status == Change::DELETED; }
    [[nodiscard]] bool is_writable() const { return (contents->flags & ARCHIVE_FL_RDONLY) == 0; }
    virtual bool check() { return true; } // This is done as part of the constructor, remove?
    virtual bool close_xxx() { return true; }
    virtual bool commit_xxx() = 0;
    virtual void commit_cleanup() = 0;
    virtual void get_last_update() = 0;
    virtual bool read_infos_xxx() = 0;
    [[nodiscard]] virtual bool want_crc() const { return true; }
    [[nodiscard]] virtual bool have_direct_file_access() const { return false; }
    ZipSourcePtr get_source(uint64_t index) { return get_source(index, 0, {}); }
    virtual ZipSourcePtr get_source(uint64_t index, uint64_t start, std::optional<uint64_t> length) = 0;
    virtual std::string get_full_filename(uint64_t index) { return ""; }
    virtual std::string get_original_filename(uint64_t index) { return ""; }

    ArchiveContentsPtr contents;
    std::vector<File> &files;
    std::string &name;
    const filetype_t filetype;
    const where_t where;
    std::vector<Change> changes;

    bool cache_changed;
    bool modified;
    
protected:
    enum GetHashesStatus {
        OK,
        READ_ERROR,
        CRC_ERROR
    };
    Archive(ArchiveType type, const std::string &name, filetype_t filetype, where_t where, int flags);
    void update_cache();

    void add_file(const std::string &filename, const Hashes *hashes, const std::unordered_map<size_t, Hashes> *detector_hashes);
    GetHashesStatus get_hashes(ZipSource *source, uint64_t length, bool eof, Hashes *hashes);
    void merge_files(const std::vector<File> &files_cache);
    
private:
    bool compute_detector_hashes(size_t index, const std::unordered_map<size_t, DetectorPtr> &detectors);
};

#endif //* HAD_ARCHIVE_H
