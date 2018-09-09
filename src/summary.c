/*
 summary.c -- store stats of the ROM set
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

#include "summary.h"
#include "util.h"

static void summary_add_file(summary_t *summary, int type, uint64_t size, quality_t status);

void
summary_free(summary_t *summary) {
    free(summary);
}


summary_t *
summary_new() {
    summary_t *summary = (summary_t *)malloc(sizeof(*summary));
    
    if (summary == NULL) {
        return NULL;
    }
    
    summary->games_good = 0;
    summary->games_partial = 0;
    summary->games_total = 0;
    
    for (int i = 0; i < TYPE_MAX; i++) {
        summary->files[i].bytes_good = 0;
        summary->files[i].bytes_total = 0;
        summary->files[i].files_good = 0;
        summary->files[i].files_total = 0;
    }
    
    return summary;
}


void
summary_add_disk(summary_t *summary, const disk_t *disk, quality_t status) {
    summary_add_file(summary, TYPE_DISK, 0, status);
}


void
summary_add_game(summary_t *summary, game_status_t status) {
    if (summary == NULL) {
        return;
    }
    
    switch (status) {
        case GS_OLD:
        case GS_CORRECT:
        case GS_FIXABLE:
            summary->games_good++;
            break;
            
        case GS_PARTIAL:
            summary->games_partial++;
            break;
            
        case GS_MISSING:
            break;
    }
    
    summary->games_total++;
}


void
summary_add_rom(summary_t *summary, int type, const file_t *rom, quality_t status) {
    // TODO: only own ROMs? (what does dumpgame /stats count?)
    
    summary_add_file(summary, type, file_size(rom), status);
}


void
summary_print(summary_t *summary, FILE *f, bool total_only) {
    static const char *ft_name[] = {"ROMs:", "Samples:", "Disks:"};

    if (summary->games_total > 0) {
        fprintf(f, "Games:  \t");
        if (!total_only) {
            if (summary->games_good > 0 || summary->games_partial == 0) {
                fprintf(f, "%" PRIu64, summary->games_good);
            }
            if (summary->games_partial > 0) {
                if (summary->games_good > 0) {
                    fprintf(f, " complete, ");
                }
                fprintf(f, "%" PRIu64 " partial", summary->games_partial);
            }
            fprintf(f, " / ");
        }
        fprintf(f, "%" PRIu64 "\n", summary->games_total);
    }
    
    for (int type = 0; type < TYPE_MAX; type++) {
        if (summary->files[type].files_total > 0) {
            fprintf(f, "%-8s\t", ft_name[type]);
            if (!total_only) {
                fprintf(f, "%" PRIu64 " / ", summary->files[type].files_good);
            }
            fprintf(f, "%" PRIu64, summary->files[type].files_total);
            
            if (summary->files[type].bytes_total > 0) {
                printf(" (");
                if (!total_only) {
                    print_human_number(f, summary->files[type].bytes_good);
                    fprintf(f, " / ");
                }
                print_human_number(f, summary->files[type].bytes_total);
                fprintf(f, ")");
            }
            fprintf(f, "\n");
        }
    }
}


static void
summary_add_file(summary_t *summary, int type, uint64_t size, quality_t status) {
    if (summary == NULL) {
        return;
    }
    
    if (status != QU_MISSING) {
        summary->files[type].bytes_good += size;
        summary->files[type].files_good++;
    }
    
    summary->files[type].bytes_total += size;
    summary->files[type].files_total++;
}
