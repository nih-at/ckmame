#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "types.h"
#include "dbl.h"
#include "funcs.h"
#include "error.h"

void *xmalloc(size_t size);



enum state
romcmp(struct rom *r1, struct rom *r2)
{
    /* r1 is important */
    if (strcasecmp(r1->name, r2->name) == 0) {
	if (r1->size == r2->size) {
	    if (r1->crc == r2->crc || r2->crc == 0 || r1->crc == 0)
		return ROM_OK;
	    else
		return ROM_CRCERR;
	}
	else if (r1->size > r2->size)
	    return ROM_LONG;
	else
	    return ROM_SHORT;
    }
    else if (r1->size == r2->size && r1->crc == r2->crc)
	return ROM_NAMERR;
    else
	return ROM_UNKNOWN;
}



char *
findzip(char *name)
{
    char *s;

    s = xmalloc(strlen(name)+10);

    sprintf(s, "roms/%s.zip", name);

    return s;
}



void
game_free(struct game *g, int fullp)
{
    int i;

    free(g->name);
    free(g->cloneof[0]);
    free(g->cloneof[1]);
    free(g->sampleof);
    for (i=0; i<g->nrom; i++)
	free(g->rom[i].name);
    for (i=0; i<g->nsample; i++)
	free(g->sample[i].name);
    if (fullp) {
	free(g->rom);
	free(g->sample);
    }

    free(g);
}



int
strpcasecmp(char **sp1, char **sp2)
{
    return strcasecmp(*sp1, *sp2);
}



static void delchecked_r(struct tree *t, int nclone, char **clone);

char **
delchecked(struct tree *t, int nclone, char **clone)
{
    char **need;

    need = (char **)xmalloc(sizeof(char *)*nclone);
    memcpy(need, clone, sizeof(char *)*nclone);

    delchecked_r(t->child, nclone, need);

    return need;
}



static void
delchecked_r(struct tree *t, int nclone, char **clone)
{
    int i, cmp;
    
    for (; t; t=t->next) {
	for (i=0; i<nclone; i++) {
	    if (clone[i]) {
		cmp = strcmp(clone[i], t->name);
		if (cmp == 0) {
		    clone[i] = NULL;
		    break;
		}
		else if (cmp > 0)
		    break;
	    }
	}
	if (t->child)
	    delchecked_r(t->child, nclone, clone);
    }
}



void
zip_free(struct zip *zip)
{
    int i;

    if (zip == NULL)
	return;
    
    free(zip->name);
    for (i=0; i<zip->nrom; i++)
	free(zip->rom[i].name);

    if (zip->nrom)
	free(zip->rom);
    free(zip);
}



char *
memmem(const char *big, int biglen, const char *little, int littlelen)
{
    int i;
    
    if (biglen < littlelen)
	return NULL;
    
    for (i=0; i<biglen-littlelen; i++)
	if (memcmp(big+i, little, littlelen)==NULL)
	    return big+i;

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
