#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "types.h"
#include "dbl.h"
#include "util.h"
#include "xmalloc.h"
#include "zip.h"

#define MAXCOMLEN        65536
#define EOCDLEN             22
#define BUFSIZE       (MAXCOMLEN+EOCDLEN)
#define LOCAL_MAGIC   "PK\3\4"
#define CENTRAL_MAGIC "PK\1\2"
#define EOCD_MAGIC    "PK\5\6"
#define DATADES_MAGIC "PK\7\8"
#define CDENTRYSIZE        36

#define READ2(a)      (*((a)++)+(*((a)++))*256)
#define READ4(a)      (READ2(a)+READ2(a)*65536)

struct zf *readcdir(FILE *fp, char *buf, char *eocd, int buflen);
struct zf *zf_new();
void zf_free(struct zf *zf);
struct zf *zip_open(char *fn);
char *readstr(char **str, int len);
int readcdentry(FILE *fp, char **cdpp, struct zf_entry *zfe, int left);
int checkcons(struct zf *zf, FILE *fp);



struct zf *
zip_open(char *fn)
{
    FILE *fp;
    char *buf, *match;
    int a, i, buflen, best;
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

    buf = (char *)xmalloc(BUFSIZE);
    
    buflen = fread(buf, 1, BUFSIZE, fp);

    if (ferror(fp)) {
	/* read error */
	free(buf);
	return NULL;
    }
    
    best = -2;
    cdir = NULL;
    match = buf;
    while ((match=memmem(match, buflen-(match-buf)-18, EOCD_MAGIC, 4))!=NULL) {
	/* found match -- check, if good */
	if ((cdirnew=readcdir(fp, buf, match, buflen)) == NULL)
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

    cdir->zn = xstrdup(fn);
    cdir->zp = fp;
    
    return cdir;
}



struct zf *
readcdir(FILE *fp, char *buf, char *eocd, int buflen)
{
    struct zf *zf;
    char *cdp;
    int i, eocdlen;

    eocdlen = buflen - (eocd - buf);
    /* check and read in end-of-central-dir entry (eocd) */
    if (eocdlen<22) {
	/* not enough bytes left for eocd */
	return NULL;
    }

    /* check for eocd magic */
    if (memcmp(eocd, EOCD_MAGIC, 4)!=0)
	return NULL;

    zf = zf_new();

    if (memcmp(eocd+4, "\1\0\1\0", 4) != 0) {
	zip_err = ZERR_MULTIDISK;
	return NULL;
    }

    cdp = eocd + 10;
    zf->nentry = READ2(cdp);
    zf->cd_size = READ4(cdp);
    zf->cd_offset = READ4(cdp);
    zf->com_size = READ2(cdp);
    zf->entry = NULL;

    if (zf->com_size != eocdlen-22) {
	/* comment size wrong -- too few or too many left after central dir */
	zf->nentry = 0;
	zf_free(zf);
	return NULL;
    }

    zf->com = memdup(eocd+22, zf->com_size);

    cdp = NULL;
    if (zf->cd_size < eocd-buf) {
	/* if buffer already read in, use it */
	cdp = eocd - zf->cd_size;
    }
    else {
	/* else go to start of cdir and read entry by entry */
	clearerr(fp);
	fseek(fp, -(zf->cd_size+zf->com_size+22), SEEK_END);
	if (ferror(fp)) {
	    /* seek error */
	    zf->nentry = 0;
	    zf_free(zf);
	    return NULL;
	}
    }

    for (i=0; i<zf->nentry; i++) {
	if (readcdentry(fp, &cdp, zf->entry+i, eocd-cdp)!=0) {
	    /* i entries have already been filled, tell zf_free
	       how many to free */
	    zf->nentry = i+1;
	    zf_free(zf);
	    return NULL;
	}
    }
    
    return zf;
}



int
readcdentry(FILE *fp, char **cdpp, struct zf_entry *zfe, int left)
{
    char *cur;
    
    if (*cdpp == NULL) {
	/* read entry from disk */
	cur = (char *)xmalloc(BUFSIZE);
	if ((fread(cur, 1, CDENTRYSIZE, fp)<CDENTRYSIZE)) {
	    free(cur);
	    return -1;
	}
	left = CDENTRYSIZE;
    }
    else
	cur = *cdpp;

    if (left < CDENTRYSIZE) {
	if (*cdpp == NULL)
	    free(cur);
	return -1;
    }
    
    /* convert buffercontents to zf_entry */
    if (memcmp(cur, CENTRAL_MAGIC, 4)!=0) {
	if (*cdpp == NULL)
	    free(cur);
	return -1;
    }
    cur += 4;

    /* fill structure */
    zfe->version_made = READ2(cur);
    zfe->version_need = READ2(cur);
    zfe->bitflags = READ2(cur);
    zfe->comp_meth = READ2(cur);
    zfe->lmtime = READ2(cur);
    zfe->lmdate = READ2(cur);

    zfe->crc = READ4(cur);
    zfe->comp_size = READ4(cur);
    zfe->uncomp_size = READ4(cur);
    
    zfe->fnlen = READ2(cur);
    zfe->eflen = READ2(cur);
    zfe->fcomlen = READ2(cur);
    zfe->disknrstart = READ2(cur);
    zfe->intatt = READ2(cur);

    zfe->extatt = READ4(cur);
    zfe->local_offset = READ4(cur);

    /* read extra data */
    if (zfe->fnlen)
	zfe->fn = readstr(&cur, zfe->fnlen);
    if (zfe->eflen)
	zfe->ef = readstr(&cur, zfe->eflen);
    if (zfe->fcomlen)
	zfe->fcom = readstr(&cur, zfe->fcomlen);
    
    if (*cdpp != NULL)
	*cdpp = cur;
    else
	free(cur);

    return 0;
}



int
checkcons(struct zf *zf, FILE *fp)
{
    /* XXX: to be written */

    return 0;
}



char *
readstr(char **str, int len)
{
    char *r;

    r = (char *)xmalloc(len);
    memcpy(r, *str, len);

    *str += len;

    return r;
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
