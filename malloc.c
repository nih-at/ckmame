#include <stdlib.h>
#include <string.h>

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



char *
xstrdup(char *str)
{
    char *p;

    if ((p=malloc(strlen(str)+1)) == NULL) {
	myerror(ERRDEF, "malloc failure");
	exit(1);
    }

    strcpy(p, str);
    
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
