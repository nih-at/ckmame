#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "types.h"
#include "dbl.h"
#include "funcs.h"
#include "zip.h"

#define MAXCOMLEN        65536
#define EOCDLEN             22
#define BUFSIZE       (MAXCOMLEN+EOCDLEN)
#define LOCAL_MAGIC   "PK\3\4"
#define CENTRAL_MAGIC "PK\1\2"
#define EOCD_MAGIC    "PK\5\6"
#define DATADES_MAGIC "PK\7\8"

#define READ2(a)      (*(a)+((a)[1])*256)
#define READ4(a)      (READ2(a)+READ2(a)*65536)		       

struct zf *readcdir(FILE *fp, char *buf, int j, char *match);
struct zf *zf_new();
void zf_free(struct zf *zf);
struct zf *z_open(char *fn);



struct zf *
z_open(char *fn)
{
    FILE *fp;
    char *buf, *match;
    int a, i, j, best;
    long ft;
    struct zf *cdir, *cdirnew;

    if ((fp=fopen(fn, "r+b"))==NULL)
	return NULL;
    clearerr(fp);
    
    i = fseek(fp, -BUFSIZE, SEEK_END);
    if (i == -1 && errno != EFBIG) {
	/* seek before start of file on my machine */
	fclose(fp);
	return NULL;
    }

    ft = ftell(fp);
    
    buf = (char *)xmalloc(sizeof(char)*BUFSIZE);
    
    j = fread(buf, 1, BUFSIZE, fp);

    if (ferror(fp)) {
	/* read error */
	free(buf);
	return NULL;
    }
    
    best = -2;
    cdir = NULL;
    match = buf;
    while ((match=memmem(match, j-(match-buf)-18, EOCD_MAGIC, 4)) != NULL) {
	/* found match -- check, if good */
	if ((cdirnew=readcdir(fp, buf, j, match)) == NULL)
	    continue;	    
	
	if (cdir) {
	    if (best == -2)
		best = checkcons(cdir, fp);
	    a = checkcons(cdirnew, fp);
	    if (best < a) {
		zf_free(cdir);
		cdir = cdirnew;
		best = a;
	    }
	    else
		zf_free(cdirnew);
	    cdirnew = NULL;
	}
	else {
	    cdir = cdirnew;
	    cdirnew = NULL;
	}
    }

    if (best < 0) {
	/* no eocd found */
	free(buf);
	zf_free(cdir);
	return NULL;
    }

    free(buf);

    return cdir;
}



struct zf *
readcdir(FILE *fp, char *buf, int j, char *match)
{
    struct zf *zf;
    char *cdp;
    int i;

    /* check and read in eocd */
    if (j-(match-buf)<22) {
	/* not enough bytes left for central dir */
	return NULL;
    }

    if (memcmp(match, EOCD_MAGIC, 4)!=0)
	return NULL;

    zf = zf_new();

    zf->nentry = READ2(match+10);
    zf->cd_size = READ4(match+12);
    zf->cd_offset = READ4(match+16);
    zf->com_size = READ2(match+20);
    zf->entry = NULL;

    if (zf->com_size != j-(match+22-buf)) {
	/* comment size wrong -- too few or too many left after central dir */
	zf->nentry = 0;
	zf_free(zf);
	return NULL;
    }

    zf->com = memdup(match+22, zf->com_size);

    cdp = NULL;
    if (zf->cd_size < match-buf) {
	/* if buffer already read in, use it */
	cdp = match - zf->cd_size;
    }
    else {
	/* else go to start of cdir and read entry by entry */
	clearerr(fp);
	fseek(fp, -(match-buf)-zf->cd_size, SEEK_END);
	if (ferror(fp)) {
	    /* seek error */
	    zf->nentry = 0;
	    zf_free(zf);
	    return NULL;
	}
    }
	    
    for (i=0; i<zf->nentry; i++) {
	if (readcdentry(fp, &cdp, zf->entry+i)!=0) {
	    /* i entries have already been filled, tell zf_free
	       how many to free */
	    zf->nentry = i+1;
	    zf_free(zf);
	    return NULL;
	}
    }
    
    return zf;
}



struct zf *
zf_new()
{
    struct zf *zf;

    zf = (struct zf *)xmalloc(sizeof(struct zf));

    zf->nentry = zf->com_size = zf->cd_size = zf->cd_offset = 0;
    zf->com = NULL;
    zf->entry = NULL;

    return zf;
}



void
zf_free(struct zf *zf)
{
    int i;
    
    if (zf == NULL)
	return;

    if (zf->com)
	free(zf->com);

    if (zf->entry) {
	for (i=0; i<zf->nentry; i++) {
	    free(zf->entry[i].fn);
	    free(zf->entry[i].ef);
	    free(zf->entry[i].fcom);
	}
	free (zf->entry);
    }

    free(zf);

    return;
}
