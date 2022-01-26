/*
 archive_dir.c -- implementation of archive from directory
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

#include "ArchiveDir.h"

// fseek
#include "compat.h"

#include "Dir.h"
#include "Exception.h"
#include "error.h"
#include "file_util.h"
#include "fix_util.h"
#include "util.h"

#include <cerrno>
#include <climits>
#include <cstring>
#include <filesystem>
#include <limits>

#include <sys/stat.h>


bool ArchiveDir::ensure_archive_dir() {
    return ensure_dir(name, false);
}


bool ArchiveDir::commit_xxx() {
    seterrinfo("", name);
    
    std::filesystem::path added_directory;
    
    try {
        added_directory = make_unique_path(std::filesystem::path(name) / ".added");
    }
    catch (...) {
        myerror(ERRZIP, "can't create temporary directory: %s", strerror(errno));
        return false;
    }
    
    try {
        for (size_t index = 0; index < files.size(); index++) {
            auto &file = files[index];
            auto &change = changes[index];
            
            if (!change.file.empty()) {
                if (!ensure_dir(added_directory, false)) {
                    throw Exception();
                }
                if (!link_or_copy(change.file, added_directory / file.name)) {
                    throw Exception();
                }
            }
            else if (change.source) {
                if (!ensure_dir(added_directory, false)) {
                    throw Exception();
                }
                copy_source(change.source.get(), added_directory / file.name);
            }
        }
    }
    catch (...) {
        std::filesystem::remove_all(added_directory);
        return false;
    }

    Commit commit(this);
        
    try {
        for (size_t index = 0; index < files.size(); index++) {
            auto &file = files[index];
            auto &change = changes[index];
            
            if (change.status == Change::DELETED) {
                commit.delete_file(get_original_filename(index));
            }
            else if (!change.file.empty() || change.source) {
                commit.rename_file(added_directory / file.name, get_full_filename(index));
            }
            else if (!change.original_name.empty()){
                commit.rename_file(get_original_filename(index), get_full_filename(index));
            }
        }
    }
    catch (...) {
        commit.undo();
        std::filesystem::remove_all(added_directory);
        return false;
    }

    commit.done();
    std::filesystem::remove_all(added_directory);

    if (is_empty()) {
        std::error_code ec;
        std::filesystem::remove(name, ec);
    }
    
    return true;
}

void ArchiveDir::commit_cleanup() {
    auto path = std::filesystem::path(name);
    
    for (auto &file : files) {
        file.mtime = get_mtime(path / file.filename());
    }
}


void ArchiveDir::copy_source(ZipSource *source, const std::filesystem::path &destination) {
    auto fout = make_shared_file(destination, "w");
    if (!fout) {
        myerror(ERRZIP, "cannot open '%s': %s", destination.c_str(), strerror(errno));
        throw Exception();
    }

    uint8_t buffer[BUFSIZ];
            
    source->open();

    uint64_t n;
    while ((n = source->read(buffer, sizeof(buffer))) > 0) {
        if (fwrite(buffer, 1, n, fout.get()) != n) {
            myerror(ERRZIP, "can't write '%s': %s", destination.c_str(), strerror(errno));
            source->close();
            throw Exception();
        }
    }

    source->close();
}


void ArchiveDir::Commit::delete_file(const std::filesystem::path &file) {
    auto destination = make_unique_path(deleted_directory / file.filename());
    
    ensure_dir(deleted_directory, false);
    rename(get_filename(file), destination);
}


void ArchiveDir::Commit::rename_file(const std::filesystem::path &source, const std::filesystem::path &destination) {
    ensure_parent_directory(destination);
    ensure_file_doesnt_exist(destination);

    rename(get_filename(source), destination);
}


void ArchiveDir::Commit::rename(const std::filesystem::path &source, const std::filesystem::path &destination) {
    std::filesystem::rename(source, destination);
    undos.push_back(Operation(destination, source));
    cleanup_directories.insert(source.parent_path());
}


void ArchiveDir::Commit::undo() {
    std::error_code ec;
    
    for (auto it = undos.rbegin(); it != undos.rend(); it++) {
        auto &operation = *it;
        
        switch (operation.type) {
            case Operation::DELETE:
                std::filesystem::remove(operation.new_name, ec);
                break;
            case Operation::RENAME:
                std::filesystem::rename(operation.old_name, operation.new_name, ec);
                break;
        }
    }
    
    std::filesystem::remove(deleted_directory, ec);
}


void ArchiveDir::Commit::done() {
    std::error_code ec;

    std::filesystem::remove_all(deleted_directory, ec);
    for (const auto &directory : cleanup_directories) {
        remove_directories_up_to(directory, archive->name);
    }
}

std::filesystem::path ArchiveDir::Commit::get_filename(const std::filesystem::path &filename) {
    auto it = renamed_files.find(filename);
    
    if (it != renamed_files.end()) {
        return it->second;
    }
    
    return filename;
}


void ArchiveDir::Commit::ensure_file_doesnt_exist(const std::filesystem::path &file) {
    if (std::filesystem::exists(file)) {
        auto new_name = make_unique_path(file);
        std::filesystem::rename(file, new_name);
        undos.push_back(Operation(new_name, file));
        renamed_files[file] = new_name;
    }
}


void ArchiveDir::Commit::ensure_parent_directory(const std::filesystem::path &file) {
    auto directory = file.parent_path();
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directory(directory);
        undos.push_back(Operation(directory));
    }
}


bool ArchiveDir::read_infos_xxx() {
    try {
	 Dir dir(name, contents->flags & ARCHIVE_FL_TOP_LEVEL_ONLY ? false : true);
	 std::filesystem::path filepath;

	 while ((filepath = dir.next()) != "") {
             if (name == filepath || name_type(filepath) == NAME_IGNORE || !std::filesystem::is_regular_file(filepath)) {
                 continue;
             }

             files.push_back(File());
             auto &f = files[files.size() - 1];
             
             f.name = filepath.string().substr(name.size() + 1);
             f.hashes.size = std::filesystem::file_size(filepath);
             // auto ftime = std::filesystem::last_write_time(filepath);
             // f.mtime = decltype(ftime)::clock::to_time_t(ftime);
             f.mtime = get_mtime(filepath);
         }
    }
    catch (...) {
	return false;
    }

    return true;
}


void ArchiveDir::get_last_update() {
    struct stat st;

    contents->size = 0;
    if (stat(name.c_str(), &st) < 0) {
        contents->mtime = 0;
        return;
    }

    contents->mtime = st.st_mtime;
}


std::string ArchiveDir::get_full_filename(uint64_t index) {
    return std::filesystem::path(name) / (files[index].name + contents->filename_extension);
}


std::string ArchiveDir::get_original_filename(uint64_t index) {
    std::string base_name;
    if (changes.size() > index) {
        base_name = changes[index].original_name;
    }
    if (base_name.empty()) {
        base_name = files[index].name;
    }
    return std::filesystem::path(name) / (base_name + contents->filename_extension);
}


ZipSourcePtr ArchiveDir::get_source(uint64_t index, uint64_t start, std::optional<uint64_t> length) {
    auto filename = get_original_filename(index);
    zip_error_t error;
    zip_error_init(&error);
    zip_source_t *source = zip_source_file_create(filename.c_str(), start, length.has_value() ? static_cast<int64_t>(length.value()) : -1, &error);
    
    if (source == nullptr) {
        throw Exception("can't open '%s': %s", filename.c_str(), zip_error_strerror(&error));
    }
    return std::make_shared<ZipSource>(source);
}


time_t ArchiveDir::get_mtime(const std::string &file) {
    struct stat st;
    
    if (stat(file.c_str(), &st) < 0) {
        return 0;
    }
    
    return st.st_mtime;
}


ArchiveDir::Commit::Commit(ArchiveDir *archive_) : archive(archive_) {
    deleted_directory = std::filesystem::path(make_unique_name(std::filesystem::path(archive->name) / ".deleted", ""));
}
