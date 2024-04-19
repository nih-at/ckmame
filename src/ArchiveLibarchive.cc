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
#include <cstring>
#include <cerrno>

#include "Exception.h"
#include "Progress.h"
#include "file_util.h"
#include "globals.h"


ArchiveLibarchive::~ArchiveLibarchive() {
    try {
        close();
    }
    catch (...) { }
}


bool ArchiveLibarchive::ensure_la() {
    if (la != nullptr) {
        return true;
    }

    la = archive_read_new();

    archive_read_support_filter_all(la);
    archive_read_support_format_all(la);

    auto error = archive_read_open_filename(la, name.c_str(), 10240);
    if (error < ARCHIVE_WARN) {
        output.error("error %s archive '%s': %s", (contents->flags & ZIP_CREATE ? "creating" : "opening"), name.c_str(), archive_error_string(la));
        archive_read_free(la);
        la = nullptr;
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
    if (la == nullptr) {
        return true;
    }

    auto error = archive_read_free(la);

    if (error < ARCHIVE_WARN) {
        /* error closing, is la still valid? */
        output.archive_error("error closing zip: %s", archive_error_string(la));
        return false;
    }

    la = nullptr;

    return true;
}


bool ArchiveLibarchive::commit_xxx() {
    auto tmpfile = make_unique_path(name);

    output.set_error_archive(name, "");

    mtimes.clear();

    if (is_empty()) {
        if (!close_xxx()) {
            return false;
        }

        std::error_code error;
        std::filesystem::remove(name, error);
        if (error) {
            output.archive_error_error_code(error, "can't remove");
            return false;
        }
        return true;
    }

    auto writer = archive_write_new();
    struct archive_entry *entry = nullptr;

    if (writer == nullptr) {
	output.archive_error("can't create archive: %s", strerror(ENOMEM));
        return false;
    }

    try {
        if (archive_write_set_format_7zip(writer) < 0) {
            throw Exception("can't create archive: %s", strerror(ENOMEM));
        }
        if (archive_write_open_filename(writer, tmpfile.c_str()) < 0) {
            throw Exception("can't create archive: %s", strerror(errno));
        }

        time_t now = time(nullptr);

        for (uint64_t index = 0; index < files.size(); index++) {
            auto &file = files[index];
            auto &change = changes[index];

	    output.set_error_archive(name, file.name);

            if (change.status == Change::DELETED) {
                continue;
            }

            time_t mtime;
            ZipSourcePtr source;

            if (change.source) {
                mtime = now;
                source = change.source;
            }
            else {
                mtime = file.mtime;
                source = get_source(index, 0, {});
            }

            mtimes.push_back(mtime);

            entry = archive_entry_new();
            if (entry == nullptr) {
                throw Exception("can't write file header: %s", strerror(ENOMEM));
            }
            archive_entry_set_pathname(entry, file.name.c_str());
            archive_entry_set_size(entry, static_cast<int64_t>(file.hashes.size));
            archive_entry_set_filetype(entry, AE_IFREG);
            archive_entry_set_perm(entry, 0644);
            archive_entry_set_mtime(entry, mtime, 0);

            if (archive_write_header(writer, entry) < 0) {
                throw Exception("can't write file header: %s", strerror(errno));
            }
            archive_entry_free(entry);
            entry = nullptr;

            write_file(writer, source);
        }

        output.set_error_archive(name, "");

        if (archive_write_close(writer) < 0) {
            throw Exception("can't write archive: %s", strerror(errno));
        }
        archive_write_free(writer);
        writer = nullptr;

        if (tmpfile != name) {
            std::error_code error;
            std::filesystem::rename(tmpfile, name, error);
            if (error) {
                throw Exception("renaming temporary file failed: %s", error.message().c_str());
            }
        }
    }
    catch (Exception &e) {
        if (entry != nullptr) {
            archive_entry_free(entry);
        }
        if (writer != nullptr) {
            archive_write_free(writer);
        }
        if (std::filesystem::exists(tmpfile)) {
            std::error_code ec;
            std::filesystem::remove(tmpfile, ec);
        }
        mtimes.clear();
        output.archive_file_error("%s", e.what());
        return false;
    }

    return close_xxx();
}


void ArchiveLibarchive::write_file(struct archive *writer, const ZipSourcePtr& source) {
    try {
        source->open();
    }
    catch (Exception &e) {
        throw Exception("can't open file: %s", e.what());
    }

    uint8_t buffer[BUFSIZ];

    while (true) {
        uint64_t n;
        Progress::update();
        try {
            n = source->read(buffer, sizeof(buffer));
        }
        catch (Exception &e) {
            source->close();
            throw Exception("can't read file: %s", e.what());
        }

        if (n == 0) {
            break;
        }

        auto ret = archive_write_data(writer, buffer, n);
        if (ret < 0) {
            source->close();
            throw Exception("can't write file: %s", strerror(errno));
        }
    }

    source->close();
}


void ArchiveLibarchive::commit_cleanup() {
    for (size_t index = 0; index < files.size(); index++) {
        files[index].mtime = mtimes[index];
    }
}


bool ArchiveLibarchive::Source::open() {
    if (archive->have_open_file) {
        output.archive_error("cannot open '%s': archive busy", archive->files[index].name.c_str());
        return false;
    }

    return archive->seek_to_entry(index);
}

bool ArchiveLibarchive::seek_to_entry(uint64_t index) {
    if (!ensure_la()) {
        return false;
    }

    if (current_index > index) {
        // rewind
        archive_read_free(la);
        la = nullptr;
        if (!ensure_la()) {
            return false;
        }
    }

    struct archive_entry *entry;
    while (current_index <= index) {
        if (!header_read) {
            if (archive_read_next_header(la, &entry) != ARCHIVE_OK) {
                output.set_error_archive(name);
                output.archive_error("cannot open '%s': %s", files[index].name.c_str(), archive_error_string(la));
                return false;
            }
            header_read = true;
        }

        if (current_index == index) {
            break;
        }

        if (archive_read_data_skip(la) != ARCHIVE_OK) {
            output.set_error_archive(name);
            output.archive_error("cannot open '%s': %s", files[index].name.c_str(), archive_error_string(la));
            return false;
        }
        header_read = false;
        current_index += 1;
    }

    return true;
}


bool ArchiveLibarchive::read_infos_xxx() {
    if (!ensure_la()) {
        return false;
    }

    output.set_error_archive(name);

    current_index = 0;
    struct archive_entry *entry;
    int ret;
    while ((ret = archive_read_next_header(la, &entry)) == ARCHIVE_OK) {

        File r;
        r.mtime = archive_entry_mtime(entry);
        r.hashes.size = static_cast<uint64_t>(archive_entry_size(entry));
        r.name = archive_entry_pathname_utf8(entry);
        r.broken = false;
        files.push_back(r);
        changes.emplace_back();

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
        output.archive_error("can't list contents: %s", archive_error_string(la));
        return false;
    }

    return true;
}


ZipSourcePtr ArchiveLibarchive::get_source(uint64_t index, uint64_t start, std::optional<uint64_t> length) {
    uint64_t actual_length = length.has_value() ? length.value() : files[index].hashes.size - start;

    auto source = new Source(this, index, start, actual_length, files[index].hashes.size);

    return std::make_shared<ZipSource>(source->get_source());
}

ArchiveLibarchive::Source::Source(ArchiveLibarchive *archive_, uint64_t index_, uint64_t start_, uint64_t length_, uint64_t file_length) : archive(archive_), index(index_), start(start_), length(length_) {
    zip_error_init(&error);
    complete_file = (start == 0 && length == file_length);
}

zip_source_t *ArchiveLibarchive::Source::get_source() {
    auto source = zip_source_function_create(callback_c, this, nullptr);
    if (!complete_file) {
        auto window_source = zip_source_window_create(source, start, (zip_int64_t)length, nullptr);
        if (window_source == nullptr) {
            zip_source_free(source);
            return nullptr;
        }
        return window_source;
    }

    return source;
}

zip_int64_t ArchiveLibarchive::Source::callback_c(void *userdata, void *data, zip_uint64_t len, zip_source_cmd_t cmd) {
    return static_cast<ArchiveLibarchive::Source *>(userdata)->callback(data, len, cmd);
}

zip_int64_t ArchiveLibarchive::Source::callback(void *data, zip_uint64_t len, zip_source_cmd_t cmd) {
    switch (cmd) {
        case ZIP_SOURCE_OPEN:
            if (!open()) {
                zip_error_set(&error, ZIP_ER_OPEN, errno);
                return -1;
            }
            return 0;

        case ZIP_SOURCE_READ: {
            Progress::update();
            auto ret = archive_read_data(archive->la, data, len);
            if (ret < 0) {
                zip_error_set(&error, ZIP_ER_READ, errno);
                return -1;
            }
            return ret;
        }

        case ZIP_SOURCE_CLOSE:
            if (archive_read_data_skip(archive->la) != ARCHIVE_OK) {
                zip_error_set(&error, ZIP_ER_READ, errno);
                return -1;
            }
            archive->current_index += 1;
            archive->have_open_file = false;
            archive->header_read = false;
            return 0;

        case ZIP_SOURCE_STAT: {
            auto st = static_cast<zip_stat_t *>(data);

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


void ArchiveLibarchive::get_last_update() {
    struct stat st;
    if (stat(name.c_str(), &st) < 0) {
        contents->size = 0;
        contents->mtime = 0;
        return;
    }

    contents->mtime = st.st_mtime;
    contents->size = static_cast<uint64_t>(st.st_size);
}
