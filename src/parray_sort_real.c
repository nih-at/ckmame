/*
  parray_sort.c -- sort array of pointers
  Copyright (C) 2005-2010 Dieter Baron and Thomas Klausner

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

#include "parray.h"
#include "util.h"


void
parray_sort_real(parray_t *pa, int omit_duplicates, int lo, int hi,
		 int (*cmp)(const void *, const void *))
{
    int n, shrink;
    void **data;

    if (lo < 0)
	lo = 0;
    if (hi == -1 || hi > parray_length(pa))
	hi = parray_length(pa);
    if (hi < lo)
	hi = lo;

    if (hi-lo < 2)
	return;

    data = pa->entry+lo;
    n = ptr_sort(data, hi-lo, omit_duplicates, cmp);
    shrink = ((hi-lo) - n);

    if (shrink > 0 && hi < parray_length(pa))
	memmove(data+n, data+n+shrink,
		(parray_length(pa)-hi) * sizeof(pa->entry[0]));

    parray_length(pa) -= shrink;
}
