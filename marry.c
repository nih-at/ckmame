#include <stdlib.h>

#include "types.h"
#include "dbl.h"
#include "xmalloc.h"
#include "romutil.h"

void
marry (struct match *rm, int count, int *noz)
{
    int i, j, now, other;
    int *z[3];
    struct match *c;

    for (i=0; i<3; i++)
	if (noz[i] > 0) {
	    z[i] = (int *)xmalloc(sizeof(int)*noz[i]);
	    for (j=0; j<noz[i]; j++)
		z[i][j] = -1;
	}
	else
	    z[i] = NULL;

    for (i=0; i<count; i++) {
	now = i;
	while ((now != -1) && (rm[now].next)) {
	    /* sometimes now gets changed */
	    c = rm[now].next;
	    if (z[c->zno][c->fno] == -1) {
		rm[now].zno = c->zno;
		rm[now].fno = c->fno;
		rm[now].where = c->where;
		rm[now].quality = c->quality;
		z[c->zno][c->fno] = now;
		now = -1;
	    }
	    else
		while (c != NULL) {
		    other = z[c->zno][c->fno];
		    if (other != -1 && matchcmp(rm[other].next, rm[now].next) >= 0) {
			/* other has the better grip on this file */
			rm[now].next = c->next;
			free(c);
			c = rm[now].next;
		    }
		    else {
			/* now grabs other's file */
			rm[now].zno = c->zno;
			rm[now].fno = c->fno;
			rm[now].where = c->where;
			rm[now].quality = c->quality;
			z[c->zno][c->fno] = now;
			/* other has to let it go */
			if (other != -1) {
			    c = rm[other].next;
			    rm[other].next = c->next;
			    rm[other].quality = ROM_UNKNOWN;
			    free(c);
			    c = rm[other].next;
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
