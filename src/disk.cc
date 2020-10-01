/*
  disk.c -- initialize / finalize disk structure
  Copyright (C) 2004-2014 Dieter Baron and Thomas Klausner

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

#include "disk.h"

bool disk_compare_merge(const disk_t *a, const disk_t *b) {
    return strcmp(disk_merged_name(a), disk_name(b)) == 0;
}

bool disk_compare_merge_hashes(const disk_t *a, const disk_t *b) {
    return disk_compare_merge(a, b) && disk_compare_hashes(a, b);
}

bool disk_compare_hashes(const disk_t *a, const disk_t *b) {
    return hashes_cmp(disk_hashes(a), disk_hashes(b)) != HASHES_CMP_MISMATCH;
}

bool disk_mergeable(const disk_t *a, const disk_t *b) {
    /* name must match (merged) name */
    if (!disk_compare_merge(a, b)) {
	return false;
    }
    /* both can be bad dumps */
    if (hashes_types(disk_hashes(a)) == 0 && hashes_types(disk_hashes(b)) == 0) {
	return true;
    }
    /* or the hashes must match */
    if (hashes_types(disk_hashes(a)) != 0 && hashes_types(disk_hashes(b)) != 0 && disk_compare_merge_hashes(a, b)) {
	return true;
    }
    return false;
}

void
disk_init(disk_t *d) {
    d->refcount = 0;
    d->name = d->merge = NULL;
    hashes_init(&d->hashes);
    d->status = STATUS_OK;
    d->where = FILE_INGAME;
}


void
disk_finalize(disk_t *d) {
    free(d->name);
    free(d->merge);
}
