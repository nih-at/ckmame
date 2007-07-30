#ifndef HAD_DAT_H
#define HAD_DAT_H

/*
  $NiH: dat.h,v 1.5 2006/05/05 10:38:51 dillo Exp $

  dat.h -- information about dat file
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

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



#include "array.h"

struct dat_entry {
    char *name;
    char *description;
    char *version;
};

typedef array_t dat_t;
typedef struct dat_entry dat_entry_t;



#define dat_free(d)		(array_free(d, dat_entry_finalize))
#define dat_entry_description(de)	((de)->description)
#define dat_entry_name(de)	((de)->name)
#define dat_entry_version(de)	((de)->version)
#define dat_get(d, i)		((dat_entry_t *)array_get((d), (i)))
#define dat_length		array_length
#define dat_description(d, i)	(dat_entry_description(dat_get((d), (i))))
#define dat_name(d, i)		(dat_entry_name(dat_get((d), (i))))
#define dat_new()		(array_new(sizeof(dat_entry_t)))
#define dat_version(d, i)	(dat_entry_version(dat_get((d), (i))))

void dat_entry_finalize(dat_entry_t *);
void dat_entry_merge(dat_entry_t *, const dat_entry_t *, const dat_entry_t *);
void dat_entry_init(dat_entry_t *);
void *dat_push(dat_t *, const dat_entry_t *, const dat_entry_t *);

#endif /* dat.h */
