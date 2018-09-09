#ifndef _HAD_SUMMARY_H
#define _HAD_SUMMARY_H

/*
 summary.h -- store stats of the ROM set
 Copyright (C) 2018 Dieter Baron and Thomas Klausner

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

#include <stdint.h>
#include <stdio.h>

#include "disk.h"
#include "file.h"
#include "types.h"

struct summary_files {
    uint64_t files_total;
    uint64_t files_good;
    uint64_t bytes_total;
    uint64_t bytes_good;
};

typedef struct summary_files summary_files_t;

struct summary {
    uint64_t games_total;
    uint64_t games_good;
    uint64_t games_partial;
    summary_files_t files[TYPE_MAX];
};

typedef struct summary summary_t;

void summary_add_disk(summary_t *summary, const disk_t *disk, quality_t status);
void summary_add_game(summary_t *summary, game_status_t status);
void summary_add_rom(summary_t *summary, int type, const file_t *rom, quality_t status);
void summary_free(summary_t *summary);
summary_t *summary_new();
void summary_print(summary_t *summary, FILE *f, bool total_only);

#endif /* _HAD_SUMMARY_H */
