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
