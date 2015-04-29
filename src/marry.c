/*
  marry.c -- pair matches with ROMs
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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

#include "funcs.h"
#include "match.h"
#include "types.h"
#include "xmalloc.h"

#define Z(m)		(z[match_zno(m)][match_fno(m)])
#define DELETE_NEXT(i)	(parray_delete(match_array_matches(ma, (i)), 1, NULL),\
			 NEXT(i))
#define NEXT(i)		((match_array_num_matches(ma, (i)) > 1)	\
			 ? match_array_get(ma, (i), 1) : NULL)
#define FIRST(i)	(match_array_get(ma, (i), 0))

void
marry(match_array_t *ma, const int *noz)
{
    int i, j, now, other;
    int *z[3];
    match_t *m;

    for (i=0; i<3; i++)
	if (noz[i] > 0) {
	    z[i] = (int *)xmalloc(sizeof(int)*noz[i]);
	    for (j=0; j<noz[i]; j++)
		z[i][j] = -1;
	}
	else
	    z[i] = NULL;

    for (i=0; i<match_array_length(ma); i++)
	match_array_sort(ma, i);

    for (i=0; i<match_array_length(ma); i++) {
	now = i;
	while ((now != -1) && (m=NEXT(now))) {
	    if (Z(m) == -1) {
		/* first match for this file; take it */
		match_copy(FIRST(now), m);
		Z(m) = now;
		now = -1;
	    }
	    else
		while (m != NULL) {
		    other = Z(m);
		    if (other != -1 && matchcmp(NEXT(other), NEXT(now)) <= 0) {
			/* other has the better grip on this file */
			m = DELETE_NEXT(now);
		    }
		    else {
			/* now grabs other's file */
			match_copy(FIRST(now), m);
			Z(m) = now;
			/* other has to let it go */
			if (other != -1) {
			    match_quality(FIRST(other)) = ROM_UNKNOWN;
			    m = DELETE_NEXT(other);
			}
			/* other has to go looking again */
			now = other;
			break;
		    }
		}
	}
    }

    for (i=0; i<3; i++)
	free(z[i]);

    /* worse choices are used for determining unused files */
    
    return;
}
