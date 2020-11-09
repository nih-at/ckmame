/*
 stats.c -- store stats of the ROM set
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

#include <stdlib.h>

#include "stats.h"
#include "types.h"
#include "util.h"

static void stats_add_file(stats_t *stats, enum filetype type, uint64_t size, quality_t status);

void
stats_free(stats_t *stats) {
    free(stats);
}


stats_t *
stats_new() {
    stats_t *stats = (stats_t *)malloc(sizeof(*stats));
    
    if (stats == NULL) {
        return NULL;
    }
    
    stats->games_good = 0;
    stats->games_partial = 0;
    stats->games_total = 0;
    
    for (int i = 0; i < TYPE_MAX; i++) {
        stats->files[i].bytes_good = 0;
        stats->files[i].bytes_total = 0;
        stats->files[i].files_good = 0;
        stats->files[i].files_total = 0;
    }
    
    return stats;
}


void
stats_add_disk(stats_t *stats, const disk_t *disk, quality_t status) {
    stats_add_file(stats, TYPE_DISK, 0, status);
}


void
stats_add_game(stats_t *stats, game_status_t status) {
    if (stats == NULL) {
        return;
    }
    
    switch (status) {
        case GS_OLD:
        case GS_CORRECT:
        case GS_FIXABLE:
            stats->games_good++;
            break;
            
        case GS_PARTIAL:
            stats->games_partial++;
            break;
            
        case GS_MISSING:
            break;
    }
    
    stats->games_total++;
}


void
stats_add_rom(stats_t *stats, enum filetype type, const file_t *rom, quality_t status) {
    // TODO: only own ROMs? (what does dumpgame /stats count?)
    
    stats_add_file(stats, type, file_size_(rom), status);
}


void
stats_print(stats_t *stats, FILE *f, bool total_only) {
    static const char *ft_name[] = {"ROMs:", "Disks:"};

    if (stats->games_total > 0) {
        fprintf(f, "Games:  \t");
        if (!total_only) {
            if (stats->games_good > 0 || stats->games_partial == 0) {
                fprintf(f, "%" PRIu64, stats->games_good);
            }
            if (stats->games_partial > 0) {
                if (stats->games_good > 0) {
                    fprintf(f, " complete, ");
                }
                fprintf(f, "%" PRIu64 " partial", stats->games_partial);
            }
            fprintf(f, " / ");
        }
        fprintf(f, "%" PRIu64 "\n", stats->games_total);
    }
    
    for (int type = 0; type < TYPE_MAX; type++) {
        if (stats->files[type].files_total > 0) {
            fprintf(f, "%-8s\t", ft_name[type]);
            if (!total_only) {
                fprintf(f, "%" PRIu64 " / ", stats->files[type].files_good);
            }
            fprintf(f, "%" PRIu64, stats->files[type].files_total);
            
            if (stats->files[type].bytes_total > 0) {
                printf(" (");
                if (!total_only) {
                    print_human_number(f, stats->files[type].bytes_good);
                    fprintf(f, " / ");
                }
                print_human_number(f, stats->files[type].bytes_total);
                fprintf(f, ")");
            }
            fprintf(f, "\n");
        }
    }
}


static void
stats_add_file(stats_t *stats, enum filetype type, uint64_t size, quality_t status) {
    if (stats == NULL) {
        return;
    }
    
    if (status != QU_MISSING) {
        stats->files[type].bytes_good += size;
        stats->files[type].files_good++;
    }
    
    stats->files[type].bytes_total += size;
    stats->files[type].files_total++;
}
