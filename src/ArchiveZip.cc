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

#include <cerrno>

#include "funcs.h"

#include "archive.h"
#include "error.h"
#include "globals.h"
#include "xmalloc.h"

#include "ArchiveZip.h"

#define BUFSIZE 8192

bool ArchiveZip::ensure_zip() {
    if (za != NULL) {
        return true;
    }

    int zip_flags = (flags & ARCHIVE_FL_CHECK_INTEGRITY) ? ZIP_CHECKCONS : 0;
    if (flags & ARCHIVE_FL_CREATE)
	zip_flags |= ZIP_CREATE;

    int err;
    if ((za = zip_open(name.c_str(), zip_flags, &err)) == NULL) {
        char errbuf[80];

        zip_error_to_str(errbuf, sizeof(errbuf), err, errno);
        myerror(ERRDEF, "error %s zip archive '%s': %s", (flags & ZIP_CREATE ? "creating" : "opening"), name.c_str(), errbuf);
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
    if (modified && ((flags & ARCHIVE_FL_RDONLY) == 0) && za != NULL && !files.empty()) {
        if (!ensure_dir(name.c_str(), 1)) {
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
	    seterrinfo(NULL, name.c_str());
	    myerror(ERRZIP, "cannot stat file %d: %s", i, zip_strerror(za));
	    continue;
	}

	file_mtime(&files[i]) = st.mtime;
    }
}


bool ArchiveZip::file_add_empty_xxx(const std::string &filename) {
    struct zip_source *source;

    if (filetype != TYPE_ROM) {
	seterrinfo(name.c_str(), NULL);
	myerror(ERRZIP, "cannot add files to disk");
	return false;
    }

    if (is_writable()) {
        if (!ensure_zip()) {
            return false;
        }

	if ((source = zip_source_buffer(za, NULL, 0, 0)) == NULL || zip_add(za, filename.c_str(), source) < 0) {
	    zip_source_free(source);
	    seterrinfo(name.c_str(), filename.c_str());
	    myerror(ERRZIPFILE, "error creating empty file: %s", zip_strerror(za));
            return false;
	}
    }

    return true;
}


void ArchiveZip::File::close() {
    zip_fclose(zf);
}


bool ArchiveZip::file_copy_xxx(uint64_t index, Archive *source_archive_, uint64_t source_index, const std::string &filename, uint64_t start, uint64_t length) {
    struct zip_source *source;

    auto source_archive = static_cast<ArchiveZip *>(source_archive_);
    
    if (!ensure_zip() || !source_archive->ensure_zip()) {
        return false;
    }

    if ((source = zip_source_zip(za, source_archive->za, source_index, ZIP_FL_UNCHANGED, start, static_cast<int64_t>(length))) == NULL || (index >= 0 ? zip_replace(za, index, source) : zip_add(za, filename.c_str(), source)) < 0) {
	zip_source_free(source);
	seterrinfo(name.c_str(), filename.c_str());
	myerror(ERRZIPFILE, "error adding '%s' from `%s': %s", file_name(&source_archive->files[source_index]), source_archive->name.c_str(), zip_strerror(za));
	return false;
    }

    return true;
}


bool ArchiveZip::file_delete_xxx(uint64_t index) {
    if (!ensure_zip()) {
	return false;
    }

    if (zip_delete(za, index) < 0) {
	seterrinfo(NULL, name.c_str());
	myerror(ERRZIP, "cannot delete '%s': %s", zip_get_name(za, index, 0), zip_strerror(za));
	return false;
    }

    return true;
}


Archive::FilePtr ArchiveZip::file_open(uint64_t index) {
    if (!ensure_zip()) {
	return NULL;
    }

    struct zip_file *zf;

    if ((zf = zip_fopen_index(za, index, 0)) == NULL) {
	seterrinfo(NULL, name.c_str());
	myerror(ERRZIP, "cannot open '%s': %s", file_name(&files[index]), zip_strerror(za));
	return NULL;
    }

    return Archive::FilePtr(new File(zf));
}


bool ArchiveZip::file_rename_xxx(uint64_t index, const std::string &filename) {
    if (!ensure_zip()) {
        return false;
    }

    if (my_zip_rename(za, index, filename.c_str()) < 0) {
	seterrinfo(NULL, name.c_str());
	myerror(ERRZIP, "cannot rename '%s' to `%s': %s", zip_get_name(za, index, 0), filename.c_str(), zip_strerror(za));
	return false;
    }

    return true;
}


const char *ArchiveZip::File::strerror() {
    return zip_file_strerror(zf);
}


bool ArchiveZip::get_last_update(time_t *last_update, uint64_t *size) {
    struct stat st;
    if (stat(name.c_str(), &st) < 0) {
	return false;
    }

    *last_update = st.st_mtime;
    *size = static_cast<uint64_t>(st.st_size);

    return true;
}


ArchiveZip::ArchiveZip(const std::string &name_, filetype_t filetype_, where_t where_, int flags_) : Archive(name_, filetype_, where_, flags_), za(NULL) {
    struct zip_stat zsb;

    if (!ensure_zip()) {
        throw(std::exception());
    }
    
    seterrinfo(NULL, name.c_str());

    zip_uint64_t n = static_cast<zip_uint64_t>(zip_get_num_entries(za, 0));
    
    for (zip_uint64_t i = 0; i < n; i++) {
	if (zip_stat_index(za, i, 0, &zsb) == -1) {
	    myerror(ERRZIP, "error stat()ing index %d: %s", i, zip_strerror(za));
	    continue;
	}

        file_t r;
	file_mtime(&r) = zsb.mtime;
	file_size_(&r) = zsb.size;
	file_name(&r) = xstrdup(zsb.name);
	file_status_(&r) = STATUS_OK;

	hashes_init(file_hashes(&r));
	file_hashes(&r)->types = HASHES_TYPE_CRC;
	file_hashes(&r)->crc = zsb.crc;
        
        files.push_back(r);

        if (detector) {
	    file_match_detector(i);
        }

        if (flags & ARCHIVE_FL_CHECK_INTEGRITY) {
	    file_compute_hashes(i, flags & ARCHIVE_FL_HASHTYPES_MASK);
        }
    }
}

bool ArchiveZip::rollback_xxx() {
    if (za == NULL) {
        return true;
    }

    if (zip_unchange_all(za) < 0) {
	return false;
    }
    
    for (uint64_t i = 0; i < files.size(); i++) {
	if (file_where(&files[i]) == FILE_ADDED) {
            files.resize(i);
	    break;
	}

        if (file_where(&files[i]) == FILE_DELETED) {
	    file_where(&files[i]) = FILE_INGAME;
        }

	if (strcmp(file_name(&files[i]), zip_get_name(za, i, 0)) != 0) {
	    free(file_name(&files[i]));
	    file_name(&files[i]) = xstrdup(zip_get_name(za, i, 0));
	}
    }

    return 0;
}
