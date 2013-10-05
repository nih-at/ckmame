/*
  array.c -- create / free array of arbitrary types
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

#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "xmalloc.h"



void
array_free(array_t *a, void (*fn)(void *))
{
    int i;
    
    if (a == NULL)
	return;

    if (fn) {
	for (i=0; i<array_length(a); i++)
	    fn(array_get(a, i));
    }

    free(a->data);
    free(a);
}



void *
array_get(const array_t *a, int i)
{
    return (void *)(a->data + a->elem_size*i);
}



int
array_index(const array_t *a, const void *key, int (*cmp)(/* const void *, const void * */))
{
    int idx;

    for (idx=0; idx<array_length(a); idx++) {
	if (cmp(array_get(a, idx), key) == 0)
	    return idx;
    }

    return -1;
}



array_t *
array_new_sized(int size, int n)
{
    array_t *a;

    if (n<0)
	n = 0;

    a = xmalloc(sizeof(*a));

    if (n == 0)
	a->data = 0;
    else
	a->data = xmalloc(n*size);
    a->elem_size = size;
    a->nentry = 0;
    a->alloc_len = n;

    return a;
}



void
array_set(array_t *a, int i, const void *d)
{
    memcpy(array_get(a, i), d, a->elem_size);
}
