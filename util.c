#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "xmalloc.h"
#include "util.h"
#include "error.h"



char *
findzip(char *name)
{
    char *s;

    s = xmalloc(strlen(name)+10);

    sprintf(s, "roms/%s.zip", name);

    return s;
}



int
strpcasecmp(char **sp1, char **sp2)
{
    return strcasecmp(*sp1, *sp2);
}



char *
memmem(const char *big, int biglen, const char *little, int littlelen)
{
    int i;
    
    if (biglen < littlelen)
	return NULL;
    
    for (i=0; i<biglen-littlelen; i++)
	if (memcmp(big+i, little, littlelen)==NULL)
	    return (char *)big+i;

    return NULL;
}



char *
memdup(const char *mem, int len)
{
    char *ret;

    ret = (char *)xmalloc(len);

    if (memcpy(ret, mem, len)==NULL)
	return NULL;

    return ret;
}
