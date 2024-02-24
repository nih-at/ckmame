/*
 unpack.c -- unpack an archive
 Copyright (C) 2024 Dieter Baron and Thomas Klausner

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

#include "config.h"
#include "compat.h"

#include <zip.h>

#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

int enter_destination(const char* destination);
int leave_destination(int dirfd);
int unpack(const char* archive, const char *destination);
int unpack_file(const char* name, struct zip* zip_archive, zip_int64_t index);
int unpack_file_with_dirs(const char* name, struct zip* zip_archive, zip_int64_t index);

const char* program_name;

int main(int argc, char *argv[]) {
    program_name = argv[0];

    if (argc != 3) {
	fprintf(stderr, "usage: %s archive destination\n", program_name);
	exit(1);
    }

    const char* destination = argv[2];
    int ret = unpack(argv[1], destination);

    if (ret != 0) {
        /* try removing destination if unpacking failed */
        (void)rmdir(destination);
    }

    exit(ret);
}

int enter_destination(const char* destination) {
    int dirfd;
    if ((dirfd = open(".", O_RDONLY)) < 0) {
        fprintf(stderr, "%s: cannot open current directory for returning to it: %s\n", program_name, strerror(errno));
        return -1;
    }
    if (mkdir(destination, 0777) != 0 && errno != EEXIST) {
        fprintf(stderr, "%s: cannot create destination '%s': %s\n", program_name, destination, strerror(errno));
        (void)close(dirfd);
        return -1;
    }
    if (chdir(destination) != 0) {
        fprintf(stderr, "%s: cannot change into destination directory '%s': %s\n", program_name, destination,
                strerror(errno));
        (void)rmdir(destination);
        (void)close(dirfd);
        return -1;
    }

    return dirfd;
}

int leave_destination(int dirfd) {
    if (fchdir(dirfd) != 0) {
        fprintf(stderr, "%s: cannot return to previous directory: %s\n", program_name, strerror(errno));
        return -1;
    }
    (void)close(dirfd);
    return 0;
}

int unpack(const char* archive, const char *destination) {
    struct zip* zip_archive;
    int err;
    int old_dir;

    if ((zip_archive = zip_open(archive, ZIP_RDONLY, &err)) == NULL) {
        zip_error_t error;
        zip_error_init_with_code(&error, err);
        fprintf(stderr, "%s: cannot open zip archive '%s': %s\n",
                program_name, archive, zip_error_strerror(&error));
        zip_error_fini(&error);
        return 1;
    }
    if ((old_dir = enter_destination(destination)) < 0) {
        (void)zip_discard(zip_archive);
        return 1;
    }
    zip_int64_t count = zip_get_num_entries(zip_archive, 0);
    for (zip_int64_t index = 0; index < count; index++) {
        const char* name = zip_get_name(zip_archive, index, 0);
        if (name[strlen(name) - 1] == '/') {
            fprintf(stderr, "%s: file name '%s' ends with slash, not supported\n", program_name, name);
            return 1;
        }
        if (unpack_file_with_dirs(name, zip_archive, index) != 0) {
            (void)zip_discard(zip_archive);
            (void)leave_destination(old_dir);
            return 1;
        }
    }
    if (zip_close(zip_archive) != 0) {
        fprintf(stderr, "%s: cannot close zip archive '%s': %s\n", program_name, archive, zip_strerror(zip_archive));
        (void)leave_destination(old_dir);
        return 1;
    }
    if (leave_destination(old_dir) != 0) {
        return 1;
    }
    return 0;
}

int unpack_file_with_dirs(const char* name, struct zip* zip_archive, zip_int64_t index) {
    char *slash;
    if ((slash = strchr(name, '/')) != NULL) {
        char directory[MAXPATHLEN];
        int old_dir;
        int ret;
        if (slash - name >= MAXPATHLEN) {
            fprintf(stderr, "%s: subdirectory path '%.*s' too long\n", program_name, (int)(slash - name), name);
            return 1;
        }
        strncpy(directory, name, slash - name);
        directory[slash - name] = '\0';
        if ((old_dir = enter_destination(directory)) < 0) {
            return 1;
        }
        ret = unpack_file_with_dirs(slash + 1, zip_archive, index);
        if (leave_destination(old_dir) != 0) {
            ret |= 1;
        }
        return ret;
    }
    else {
        return unpack_file(name, zip_archive, index);
    }
}

int unpack_file(const char* name, struct zip* zip_archive, zip_int64_t index) {
    struct zip_file* zip_entry;
    FILE* out;
    char buf[8192];
    zip_int64_t n;
    size_t n_out;
    if ((zip_entry=zip_fopen_index(zip_archive, index, 0)) == NULL) {
        fprintf(stderr, "%s: error opening '%s' in archive: %s\n", program_name, name, zip_strerror(zip_archive));
        return 1;
    }
    if ((out = fopen(name, "w+x")) == NULL) {
        fprintf(stderr, "%s: error creating output file '%s': %s\n", program_name, name, strerror(errno));
        (void)zip_fclose(zip_entry);
        return 1;
    }
    while ((n = zip_fread(zip_entry, buf, sizeof(buf))) > 0) {
        if ((n_out = fwrite(buf, 1, n, out)) != n) {
            fprintf(stderr, "%s: error writing to output file '%s' (%lld != %zd): %s\n", program_name, name, (long long)n, n_out, strerror(errno));
            (void)fclose(out);
            (void)zip_fclose(zip_entry);
            return 1;
        }
    }
    if (n != 0) {
        fprintf(stderr, "%s: error reading from file '%s' in archive: %s\n", program_name, name,
                zip_file_strerror(zip_entry));
        (void)fclose(out);
        (void)zip_fclose(zip_entry);
        return 1;
    }
    if (fclose(out) != 0) {
        fprintf(stderr, "%s: error closing output file '%s': %s\n", program_name, name, strerror(errno));
        (void)zip_fclose(zip_entry);
        return 1;
    }
    if (zip_fclose(zip_entry) != 0) {
        fprintf(stderr, "%s: error closing '%s' in archive: %s\n", program_name, name, zip_strerror(zip_archive));
        return 1;
    }
    return 0;
}
