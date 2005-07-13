/*
  $NiH: marry.c,v 1.1 2005/07/04 21:54:51 dillo Exp $

  marry.c -- pair matches with ROMs
  Copyright (C) 1999, 2004, 2005 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

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
