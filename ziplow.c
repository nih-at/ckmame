#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "types.h"
#include "dbl.h"
#include "util.h"
#include "xmalloc.h"
#include "zip.h"

#undef NEVER 

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
struct zf *zf_new(void);
int zf_free(struct zf *zf);
struct zf *zip_open(char *fn);
char *readstr(FILE *fp, int len);
int readcdentry(FILE *fp, struct zf_entry *zfe, char **cdpp, int left,
		int readp);
int checkcons(struct zf *zf, FILE *fp);



char *
readstr(FILE *fp, int len)
{
    char *r;

    r = (char *)xmalloc(len);
    if (fread(r, 1, len, fp)<len) {
	free(r);
	return NULL;
    }

    return r;
}



struct zf *
zip_open(char *fn)
{
    FILE *fp;
    char *buf, *match;
    int a, i, buflen, best;
    struct zf *cdir, *cdirnew;

    if ((fp=fopen(fn, "rb"))==NULL)
	return NULL;

    clearerr(fp);
    i = fseek(fp, -BUFSIZE, SEEK_END);
    if (i == -1 && errno != EFBIG) {
	/* seek before start of file on my machine */
	fclose(fp);
	return NULL;
    }

    buf = (char *)xmalloc(BUFSIZE);

    clearerr(fp);
    buflen = fread(buf, 1, BUFSIZE, fp);

    if (ferror(fp)) {
	/* read error */
	free(buf);
	fclose(fp);
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
	fclose(fp);
	return NULL;
    }

    free(buf);

    cdir->zn = xstrdup(fn);
    cdir->zp = fp;
    
    return cdir;
}



#ifdef NEVER

int
zip_close(struct zf *zf)
{
    int i, count, td;
    char *temp;
    FILE *tfp;

    if (zf->changes == 0)
	return zf_free(zf);

    /* XXX: create better random names */
    if ((td=open("tempXxXx", O_RDWR|O_CREAT|O_BINARY, 0))==0)
	return -1;

    tfp = fdopen(td);
    
    count = 0;
    if (zf->entry) {
	for (i=0; i<zf->nentry; i++) {
	    switch (zf->entry[i].state) {
	    case Z_UNCHANGED:
		/* XXX: syntax? */
		copy_verbose(zf->fp, zf->entry[i], tfp);
		break;
	    case Z_DELETED:
		/* XXX: ok? */
		break;
	    case Z_REPLACED:
	    case Z_ADDED:
		/* copy_verbose(zf->entry[i] */
		/* XXX: don't know how to handle that nicely yet */
		break;
	    case Z_RENAMED:
		/* XXX: something missing */
		break;
	    default:
		/* shouldn't happen */
		break;
	    }
	}
    }
    
    zf_free(zf);

    return 0;
}

#endif /* NEVER */



struct zf *
readcdir(FILE *fp, char *buf, char *eocd, int buflen)
{
    struct zf *zf;
    char *cdp;
    int i, comlen, readp;

    comlen = buf + buflen - eocd - EOCDLEN;
    if (comlen < 0) {
	/* not enough bytes left for comment */
	return NULL;
    }

    /* check for end-of-central-dir magic */
    if (memcmp(eocd, EOCD_MAGIC, 4) != 0)
	return NULL;

    zf = zf_new();

    if (memcmp(eocd+4, "\1\0\1\0", 4) != 0) {
	zip_err = ZERR_MULTIDISK;
	return NULL;
    }

    cdp = eocd + 8;
    /* number of cdir-entries on this disk */
    i = READ2(cdp);
    /* number of cdir-entries */
    zf->nentry = READ2(cdp);
    zf->cd_size = READ4(cdp);
    zf->cd_offset = READ4(cdp);
    zf->com_size = READ2(cdp);
    zf->entry = NULL;

    if ((zf->com_size != comlen) || (zf->nentry != i)) {
	/* comment size wrong -- too few or too many left after central dir */
	/* or number of cdir-entries on this disk != number of cdir-entries */
	zf_free(zf);
	return NULL;
    }

    zf->com = memdup(eocd+EOCDLEN, zf->com_size);

    cdp = eocd;
    if (zf->cd_size < eocd-buf) {
	/* if buffer already read in, use it */
	readp = 0;
	cdp = eocd - zf->cd_size;
    }
    else {
	/* go to start of cdir and read it entry by entry */
	readp = 1;
	clearerr(fp);
	fseek(fp, -(zf->cd_size+zf->com_size+EOCDLEN), SEEK_END);
	if (ferror(fp) || (ftell(fp) != zf->cd_offset)) {
	    /* seek error or offset of cdir wrong */
	    zf_free(zf);
	    return NULL;
	}
    }

    zf->entry = (struct zf_entry *)xmalloc(sizeof(struct zf_entry)
					   *zf->nentry);
    for (i=0; i<zf->nentry; i++) {
	zf->entry[i].fn = NULL;
	zf->entry[i].ef = NULL;
	zf->entry[i].fcom = NULL;
    }
    
    for (i=0; i<zf->nentry; i++) {
	if ((readcdentry(fp, zf->entry+i, &cdp, eocd-cdp, readp)) == NULL) {
	    /* i entries have already been filled, tell zf_free
	       how many to free */
	    zf_free(zf);
	    return NULL;
	}
    }
    
    return zf;
}



int
readcdentry(FILE *fp, struct zf_entry *zfe, char **cdpp, int left, int readp)
{
    char buf[CDENTRYSIZE];
    char *cur;
    
    if (readp) {
	/* read entry from disk */
	if ((fread(buf, 1, CDENTRYSIZE, fp)<CDENTRYSIZE))
	    return -1;
	left = CDENTRYSIZE;
	cur = buf;
    }
    else {
	cur = *cdpp;
	if (left < CDENTRYSIZE)
	    return -1;
    }
    
    if (memcmp(cur, CENTRAL_MAGIC, 4)!=0)
	return -1;

    cur += 4;

    /* convert buffercontents to zf_entry */
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

    if (left < CDENTRYSIZE+zfe->fnlen+zfe->eflen+zfe->fcomlen) {
	if (readp) {
	    if (zfe->fnlen)
		zfe->fn = readstr(fp, zfe->fnlen);
	    if (zfe->eflen)
		zfe->ef = readstr(fp, zfe->eflen);
	    if (zfe->fcomlen)
		zfe->fcom = readstr(fp, zfe->fcomlen);
	}
	else {
	    /* can't get more bytes if not allowed to read */
	    return -1;
	}
    }
    
    return 0;
}



int
checkcons(struct zf *zf, FILE *fp)
{
    /* XXX: to be written */

    return 0;
}



struct zf *
zf_new(void)
{
    struct zf *zf;

    zf = (struct zf *)xmalloc(sizeof(struct zf));

    zf->zn = zf->com = NULL;
    zf->zp = NULL;
    zf->nentry = zf->com_size = zf->changes = 0;
    zf->cd_size = zf->cd_offset = 0;
    zf->entry = NULL;

    return zf;
}



int
zf_free(struct zf *zf)
{
    int i, ret;

    if (zf == NULL)
	return 0;

    if (zf->zn)
	free(zf->zn);

    if (zf->zp)
	ret = fclose(zf->zp);

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

    return ret;
}
