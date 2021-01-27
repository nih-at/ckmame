/*
 archive_zip.c -- implementation of archive from zip
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

#include "ArchiveZip.h"

#include <cerrno>


#include "archive.h"
#include "error.h"
#include "globals.h"
#include "util.h"
#include "zip_util.h"


#define BUFSIZE 8192

bool ArchiveZip::ensure_zip() {
    if (za != NULL) {
        return true;
    }

    int zip_flags = (contents->flags & ARCHIVE_FL_CHECK_INTEGRITY) ? ZIP_CHECKCONS : 0;
    if (contents->flags & ARCHIVE_FL_CREATE)
	zip_flags |= ZIP_CREATE;

    int err;
    if ((za = zip_open(name.c_str(), zip_flags, &err)) == NULL) {
        char errbuf[80];

        zip_error_to_str(errbuf, sizeof(errbuf), err, errno);
        myerror(ERRDEF, "error %s zip archive '%s': %s", (contents->flags & ZIP_CREATE ? "creating" : "opening"), name.c_str(), errbuf);
	return false;
    }
    
    return true;
}


bool ArchiveZip::check() {
    return ensure_zip();
}


bool ArchiveZip::close_xxx() {
    if (za == NULL) {
        return true;
    }

    if (zip_close(za) < 0) {
	/* error closing, so zip is still valid */
	myerror(ERRZIP, "error closing zip: %s", zip_strerror(za));

	/* TODO: really do this here? */
	/* discard all changes and close zipfile */
	zip_discard(za);
        za = NULL;
        return false;
    }

    za = NULL;

    return true;
}


bool ArchiveZip::commit_xxx() {
    if (modified && ((contents->flags & ARCHIVE_FL_RDONLY) == 0) && za != NULL && !files.empty()) {
        if (!ensure_dir(name, true)) {
	    return false;
        }
    }

    return close_xxx();
}


void ArchiveZip::commit_cleanup() {
    if (files.empty()) {
	return;
    }

    ensure_zip();

    for (uint64_t i = 0; i < files.size(); i++) {
	struct zip_stat st;

	if (zip_stat_index(za, i, 0, &st) < 0) {
	    seterrinfo("", name);
	    myerror(ERRZIP, "cannot stat file %d: %s", i, zip_strerror(za));
	    continue;
	}

        files[i].mtime = st.mtime;
    }
}


bool ArchiveZip::file_add_empty_xxx(const std::string &filename) {
    struct zip_source *source;

    if (filetype != TYPE_ROM) {
	seterrinfo(name);
	myerror(ERRZIP, "cannot add files to disk");
	return false;
    }

    if (is_writable()) {
        if (!ensure_zip()) {
            return false;
        }

	if ((source = zip_source_buffer(za, NULL, 0, 0)) == NULL || zip_add(za, filename.c_str(), source) < 0) {
	    zip_source_free(source);
	    seterrinfo(name, filename);
	    myerror(ERRZIPFILE, "error creating empty file: %s", zip_strerror(za));
            return false;
	}
    }

    return true;
}


void ArchiveZip::ArchiveFile::close() {
    zip_fclose(zf);
}


int64_t ArchiveZip::ArchiveFile::read(void *data, uint64_t length) {
    return zip_fread(zf, data, length);
}


bool ArchiveZip::file_copy_xxx(std::optional<uint64_t> index, Archive *source_archive_, uint64_t source_index, const std::string &filename, uint64_t start, std::optional<uint64_t> length_) {
    struct zip_source *source;

    auto source_archive = static_cast<ArchiveZip *>(source_archive_);
    
    if (!ensure_zip() || !source_archive->ensure_zip()) {
        return false;
    }
    
    // TODO: overflow check
    int64_t length = length_.has_value() ? static_cast<int64_t>(length_.value()) : -1;

    if ((source = zip_source_zip(za, source_archive->za, source_index, ZIP_FL_UNCHANGED, start, length)) == NULL || (index.has_value() ? zip_replace(za, index.value(), source) : zip_add(za, filename.c_str(), source)) < 0) {
	zip_source_free(source);
	seterrinfo(name, filename);
	myerror(ERRZIPFILE, "error adding '%s' from `%s': %s", source_archive->files[source_index].name.c_str(), source_archive->name.c_str(), zip_strerror(za));
	return false;
    }

    return true;
}


bool ArchiveZip::file_delete_xxx(uint64_t index) {
    if (!ensure_zip()) {
	return false;
    }

    if (zip_delete(za, index) < 0) {
	seterrinfo("", name);
	myerror(ERRZIP, "cannot delete '%s': %s", zip_get_name(za, index, 0), zip_strerror(za));
	return false;
    }

    return true;
}


Archive::ArchiveFilePtr ArchiveZip::file_open(uint64_t index) {
    if (!ensure_zip()) {
	return NULL;
    }

    struct zip_file *zf;

    if ((zf = zip_fopen_index(za, index, 0)) == NULL) {
	seterrinfo("", name);
	myerror(ERRZIP, "cannot open '%s': %s", files[index].name.c_str(), zip_strerror(za));
	return NULL;
    }

    return Archive::ArchiveFilePtr(new ArchiveFile(zf));
}


bool ArchiveZip::file_rename_xxx(uint64_t index, const std::string &filename) {
    if (!ensure_zip()) {
        return false;
    }

    if (my_zip_rename(za, index, filename.c_str()) < 0) {
	seterrinfo("", name);
	myerror(ERRZIP, "cannot rename '%s' to `%s': %s", zip_get_name(za, index, 0), filename.c_str(), zip_strerror(za));
	return false;
    }

    return true;
}


const char *ArchiveZip::ArchiveFile::strerror() {
    return zip_file_strerror(zf);
}


void ArchiveZip::get_last_update() {
    struct stat st;
    if (stat(name.c_str(), &st) < 0) {
        contents->size = 0;
        contents->mtime = 0;
        return;
    }

    contents->mtime = st.st_mtime;
    contents->size = static_cast<uint64_t>(st.st_size);
}


bool ArchiveZip::read_infos_xxx() {
    struct zip_stat zsb;

    if (!ensure_zip()) {
        return false;
    }
    
    seterrinfo("", name);

    zip_uint64_t n = static_cast<zip_uint64_t>(zip_get_num_entries(za, 0));
    
    for (zip_uint64_t i = 0; i < n; i++) {
	if (zip_stat_index(za, i, 0, &zsb) == -1) {
	    myerror(ERRZIP, "error stat()ing index %d: %s", i, zip_strerror(za));
	    continue;
	}

        File r;
        r.mtime = zsb.mtime;
	r.size = zsb.size;
	r.name = zsb.name;
        r.status = STATUS_OK;
        r.hashes.set_crc(zsb.crc);
        
        files.push_back(r);

        if (detector) {
	    file_match_detector(i);
        }

        if (contents->flags & ARCHIVE_FL_CHECK_INTEGRITY) {
	    file_compute_hashes(i, contents->flags & ARCHIVE_FL_HASHTYPES_MASK);
        }
    }
    
    return true;
}


bool ArchiveZip::rollback_xxx() {
    if (za == NULL) {
        return true;
    }

    if (zip_unchange_all(za) < 0) {
	return false;
    }
    
    for (uint64_t i = 0; i < files.size(); i++) {
	if (files[i].where == FILE_ADDED) {
            files.resize(i);
	    break;
	}

        if (files[i].where == FILE_DELETED) {
	    files[i].where = FILE_INGAME;
        }

        if (files[i].name != zip_get_name(za, i, 0)) {
            files[i].name = zip_get_name(za, i, 0);
	}
    }

    return true;
}
