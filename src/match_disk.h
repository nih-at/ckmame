#ifndef HAD_MATCH_DISK_H
#define HAD_MATCH_DISK_H

/*
  $NiH: match_disk.h,v 1.3 2006/05/01 21:09:11 dillo Exp $

  match_disk.h -- matching files with disks
  Copyright (C) 1999-2005 Dieter Baron and Thomas Klausner

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



#include "disk.h"
#include "parray.h"
#include "types.h"

struct match_disk {
    char *name;
    hashes_t hashes;
    quality_t quality;
    where_t where;
};

typedef struct match_disk match_disk_t;

typedef array_t match_disk_array_t;

#define match_disk_array_free(ma)	\
	(array_free((ma), (void (*)())match_disk_finalize))
#define match_disk_array_get(ma, i)	\
	((match_disk_t *)array_get((ma), (i)))
#define match_disk_array_new(n)		\
	(array_new_length(sizeof(match_disk_t), n,	\
			  (void (*)())match_disk_init))

#define match_disk_array_length	array_length

#define match_disk_hashes(m)	(&(m)->hashes)
#define match_disk_name(m)	((m)->name)
#define match_disk_quality(m)	((m)->quality)
#define match_disk_where(m)	((m)->where)



void match_disk_finalize(match_disk_t *);
void match_disk_init(match_disk_t *);
void match_disk_set_source(match_disk_t *, const disk_t *);

#endif /* match_disk.h */
