#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "xmalloc.h"
#include "util.h"
#include "error.h"



char *
findzip(char *name, int sample)
{
    char *s;

    /* XXX: search path */

    s = xmalloc(strlen(name)+13);

    sprintf(s, "%s/%s.zip", (sample ? "samples" : "roms"), name);

    return s;
}



int
strpcasecmp(char **sp1, char **sp2)
{
    return strcasecmp(*sp1, *sp2);
}



unsigned char *
memmem(const unsigned char *big, int biglen, const unsigned char *little, 
       int littlelen)
{
    int i;
    
    if (biglen < littlelen)
	return NULL;
    
    for (i=0; i<=biglen-littlelen; i++)
	if (memcmp(big+i, little, littlelen)==0)
	    return (unsigned char *)big+i;

    return NULL;
}



void *
memdup(const void *mem, int len)
{
    void *ret;

    ret = xmalloc(len);

    if (memcpy(ret, mem, len)==NULL)
	return NULL;

    return ret;
}
