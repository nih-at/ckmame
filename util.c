#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xmalloc.h"
#include "util.h"
#include "error.h"

#define MAXROMPATH 128

#define DEFAULT_ROMPATH "/usr/local/share/games/xmame" /* XXX: autoconfed */

static char *rompath[MAXROMPATH] = { NULL };
static int rompath_init = 0;

static void init_rompath(void);



char *
findzip(char *name, int sample)
{
    int i;
    char b[8192];
    struct stat st;

    if (rompath_init == 0)
	init_rompath();

    for (i=0; rompath[i]; i++) {
	sprintf(b, "%s/%s/%s.zip",
		rompath[i], (sample ? "samples" : "roms"), name);
	if (stat(b, &st) == 0)
	    return xstrdup(b);
    }
    
    return NULL;
}



static void
init_rompath(void)
{
    int i, after;
    char *s, *e;

    /* skipping components placed via command line options */
    for (i=0; rompath[i]; i++)
	;

    if ((e = getenv("ROMPATH"))) {
	s = xstrdup(e);

	after = 0;
	if (s[0] == ':')
	    rompath[i++] = DEFAULT_ROMPATH;
	else if (s[strlen(s)-1] == ':')
	    after = 1;
	
	for (e=strtok(s, ":"); e; e=strtok(NULL, ":"))
	    rompath[i++] = e;

	if (after)
	    rompath[i++] = DEFAULT_ROMPATH;
    }
    else
	rompath[i++] = DEFAULT_ROMPATH;

    rompath[i] = NULL;

    rompath_init = 1;
}



int
strpcasecmp(char **sp1, char **sp2)
{
    return strcasecmp(*sp1, *sp2);
}
