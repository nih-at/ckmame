/*
 ArchiveLibarchive.cc -- implementation of archive via libarchive
 Copyright (C) 1999-2015 Dieter Baron and Thomas Klausner

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

#include "ArchiveLibarchive.h"

#include <archive_entry.h>
#include <errno.h>

#include "error.h"


bool ArchiveLibarchive::ensure_la() {
    if (la != NULL) {
        return true;
    }
    
    la = archive_read_new();
    
    archive_read_support_filter_all(la);
    archive_read_support_format_all(la);
    
    auto error = archive_read_open_filename(la, name.c_str(), 10240);
    if (error < ARCHIVE_WARN) {
        myerror(ERRDEF, "error %s archive '%s': %s", (contents->flags & ZIP_CREATE ? "creating" : "opening"), name.c_str(), archive_error_string(la));
        archive_read_free(la);
        la = NULL;
        return false;
    }
    
    current_index = 0;
    header_read = false;
    have_open_file = false;

    return true;
}


bool ArchiveLibarchive::check() {
    return ensure_la();
}

bool ArchiveLibarchive::close_xxx() {
    if (la == NULL) {
        return true;
    }

    // TODO: for modify also close zip archive

    auto error = archive_read_free(la);

    if (error < ARCHIVE_WARN) {
        /* error closing, is la still valid? */
        myerror(ERRZIP, "error closing zip: %s", archive_error_string(la));
        return false;
    }

    la = NULL;

    return true;
}


bool ArchiveLibarchive::commit_xxx() {
    // TODO: handle modifications

    return close_xxx();
}


void ArchiveLibarchive::commit_cleanup() {
    // TODO: implement for modify
}


void ArchiveLibarchive::ArchiveFile::close() {
    archive->current_index += 1;
    archive->have_open_file = false;
    return;
}


int64_t ArchiveLibarchive::ArchiveFile::read(void *data, uint64_t length) {
    return archive_read_data(la, data, length);
}


Archive::ArchiveFilePtr ArchiveLibarchive::file_open(uint64_t index) {
    if (have_open_file) {
        myerror(ERRZIP, "cannot open '%s': archive busy", files[index].name.c_str());
        return NULL;
    }
    
    if (!ensure_la()) {
        return NULL;
    }
    

    if (current_index > index) {
        // rewind
        archive_read_free(la);
        la = NULL;
        if (!ensure_la()) {
            return NULL;
        }
    }
    
    struct archive_entry *entry;
    while (current_index < index) {
        if (!header_read) {
            if (archive_read_next_header(la, &entry) != ARCHIVE_OK) {
                seterrinfo("", name);
                myerror(ERRZIP, "cannot open '%s': %s", files[index].name.c_str(), archive_error_string(la));
                return NULL;
            }
        }
        if (archive_read_data_skip(la) != ARCHIVE_OK) {
            seterrinfo("", name);
            myerror(ERRZIP, "cannot open '%s': %s", files[index].name.c_str(), archive_error_string(la));
            return NULL;
        }
        header_read = false;
        current_index += 1;
    }
    
    if (!header_read) {
        if (archive_read_next_header(la, &entry) != ARCHIVE_OK) {
            seterrinfo("", name);
            myerror(ERRZIP, "cannot open '%s': %s", files[index].name.c_str(), archive_error_string(la));
            return NULL;
        }
        header_read = true;
    }
        
    return Archive::ArchiveFilePtr(new ArchiveFile(this, la));
}


const char *ArchiveLibarchive::ArchiveFile::strerror() {
    return archive_error_string(la);
}


bool ArchiveLibarchive::read_infos_xxx() {
    contents->flags |= ARCHIVE_FL_RDONLY;
    
    if (!ensure_la()) {
        return false;
    }
    
    seterrinfo("", name);
    
    current_index = 0;
    struct archive_entry *entry;
    int ret;
    while ((ret = archive_read_next_header(la, &entry)) == ARCHIVE_OK) {

        File r;
        r.mtime = archive_entry_mtime(entry);
        r.size = static_cast<uint64_t>(archive_entry_size(entry));
        r.name = archive_entry_pathname_utf8(entry);
        r.status = STATUS_OK;
        files.push_back(r);

        header_read = true;
        file_ensure_hashes(current_index, Hashes::TYPE_ALL);
        // current_index is increased by reading and closing the file.
        
#if 0
        // TODO: only read file once, and compute full and detector hashes at once
        if (detector) {
            file_match_detector(i);
        }
#endif
    }
    if (ret != ARCHIVE_EOF) {
        myerror(ERRZIP, "can't list contents: %s", archive_error_string(la));
        return false;
    }
    
    return true;
}


bool ArchiveLibarchive::rollback_xxx() {
    // TODO: implement for modify

    return true;
}


zip_source_t *ArchiveLibarchive::get_source(zip_t *destination_archive, uint64_t index, uint64_t start, std::optional<uint64_t> length) {
    uint64_t actual_length = length.has_value() ? length.value() : files[index].size - start;
    
    auto source = new Source(this, index, start, actual_length);
    
    return source->get_source(destination_archive);
}

zip_source_t *ArchiveLibarchive::Source::get_source(zip_t *za) {
    return zip_source_function(za, callback_c, this);
}

zip_int64_t ArchiveLibarchive::Source::callback_c(void *userdata, void *data, zip_uint64_t len, zip_source_cmd_t cmd) {
    return static_cast<ArchiveLibarchive::Source *>(userdata)->callback(data, len, cmd);
}

zip_int64_t ArchiveLibarchive::Source::callback(void *data, zip_uint64_t len, zip_source_cmd_t cmd) {
    switch (cmd) {
        case ZIP_SOURCE_OPEN:
            file = archive->file_open(index);
            
            if (file == NULL) {
                zip_error_set(&error, ZIP_ER_OPEN, errno);
                return -1;
            }
            return 0;
            
        case ZIP_SOURCE_READ: {
            auto ret = file->read(data, len);
            if (ret < 0) {
                zip_error_set(&error, ZIP_ER_READ, errno);
                return -1;
            }
            return ret;
        }
            
        case ZIP_SOURCE_CLOSE:
            file->close();
            file = nullptr;
            return 0;
            
        case ZIP_SOURCE_STAT: {
            zip_stat_t *st = static_cast<zip_stat_t *>(data);
            
            st->valid = ZIP_STAT_SIZE | ZIP_STAT_COMP_SIZE | ZIP_STAT_COMP_METHOD;
            st->comp_method = ZIP_CM_STORE;
            st->comp_size = length;
            st->size = length;

            return 0;
        }
            
        case ZIP_SOURCE_ERROR:
            return zip_error_to_data(&error, data, len);
            
        case ZIP_SOURCE_FREE:
            delete this;
            return 0;

        case ZIP_SOURCE_SUPPORTS:
            return ZIP_SOURCE_SUPPORTS_READABLE;
            
        default:
            zip_error_set(&error, ZIP_ER_OPNOTSUPP, 0);
            return -1;
    }
}

