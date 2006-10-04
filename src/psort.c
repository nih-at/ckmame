/*
  $NiH: psort.c,v 1.1 2005/07/13 17:42:20 dillo Exp $

  psort.c -- sort pointers, opionally removing duplicates
  Copyright (C) 2005 Dieter Baron and Thomas Klausner
  Copyright (C) Wikipedia

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
