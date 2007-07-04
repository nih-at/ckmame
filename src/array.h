#ifndef _HAD_ARRAY_H
#define _HAD_ARRAY_H

/*
  $NiH: array.h,v 1.5 2006/05/06 16:46:12 dillo Exp $

  array.h -- array of arbitrary types
  Copyright (C) 2005-2006 Dieter Baron and Thomas Klausner

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



struct array {
    char *data;
    int elem_size;
    int nentry;
    int alloc_len;
};

typedef struct array array_t;



#define array_length(a)		((a)->nentry)
#define array_new(s)		(array_new_sized((s), 0))

/* function arguments not specified to avoid lots of casts */
void array_delete(array_t *, int, void (*)(/* void * */));
void array_free(array_t *, void (*)(/* void * */));
void *array_get(const array_t *, int);
void *array_grow(array_t *, void (*)(/* void * */));
array_t *array_new_sized(int, int);
array_t *array_new_length(int, int, void (*)(/* void * */));
void *array_push(array_t *, void *);
void array_set(array_t *, int, const void *);
void array_truncate(array_t *, int, void (*)(/* void * */));

#endif /* array.h */
