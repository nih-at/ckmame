#include <stdlib.h>

#include "error.h"



void *
xmalloc(size_t size)
{
    void *p;

    if ((p=malloc(size)) == NULL) {
	myerror(ERRDEF, "malloc failure");
	exit(1);
    }

    return p;
}



void *
xrealloc(void *p, size_t size)
{
    if (p == NULL)
	return xmalloc(size);

    if ((p=realloc(p, size)) == NULL) {
	myerror(ERRDEF, "malloc failure");
	exit(1);
    }

    return p;
}
