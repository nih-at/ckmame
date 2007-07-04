#ifndef HAD_DISK_H
#define HAD_DISK_H

/*
  $NiH: disk.h,v 1.5 2006/10/04 17:36:43 dillo Exp $

  disk.h -- information about one disk
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

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



#include "hashes.h"
#include "types.h"

struct disk {
    int id;
    int refcount;
    char *name;
    char *merge;
    hashes_t hashes;
    status_t status;
};

typedef struct disk disk_t;

#define DISK_FL_CHECK_INTEGRITY	0x2
#define DISK_FL_QUIET		0x4



#define disk_hashes(d)	(&(d)->hashes)
#define disk_status(d)	((d)->status)
#define disk_merge(d)	((d)->merge)
#define disk_name(d)	((d)->name)
#define disk_id(d)	((d)->id)

#define disk_by_id(i)	((disk_t *)memdb_get_ptr_by_id(i))

void disk_finalize(disk_t *);
void disk_free(disk_t *);
void disk_init(disk_t *);
disk_t *disk_new(const char *, int);
void disk_real_free(disk_t *);

#endif /* disk.h */
