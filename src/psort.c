/*
  $NiH: psort.c,v 1.1 2005/07/13 17:42:20 dillo Exp $

  psort.c -- sort pointers, opionally removing duplicates
  Copyright (C) 2005 Dieter Baron and Thomas Klausner
  Copyright (C) Wikipedia

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



#include "util.h"

/*
  Implementation adapted from Wikipedia
	http://en.wikipedia.org/wiki/Heapsort
*/

#define HEAP(j)		(data[(length-1) - (j)])

int
psort(void **data, int length, int omit_duplicates,
      int (*cmp)(const void *, const void *))
{
    int heap_size, array_end, sort_node, node, child;
    void *p;

    array_end = 0;
    heap_size = length;
    sort_node = heap_size/2;

    for (;;) {
	if (sort_node > 0)
	    p = HEAP(--sort_node);
	else {
	    p = HEAP(--heap_size);
	    /* copy smallest element from heap to sorted array */
	    if (array_end == 0 || !omit_duplicates
		|| cmp(data[array_end-1], HEAP(0)) != 0) {
		data[array_end++] = HEAP(0);
	    }
	    /* if heap is empty, we're done */
	    if (heap_size == 0)
		return array_end;
	}

	node = sort_node;
	child = node*2 + 1;

	while (child < heap_size) {
	    if (child+1 < heap_size && cmp(HEAP(child+1), HEAP(child)) < 0)
		child++;

	    if (cmp(HEAP(child), p) < 0) {
		HEAP(node) = HEAP(child);
		node = child;
		child = node*2 + 1;
	    }
	    else
		break;
	}
	HEAP(node) = p;
    }
}
