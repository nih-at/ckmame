#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "ziplow.h"
#include "zip.h"

#include "util.h"
#include "xmalloc.h"

#define MAXCOMLEN        65536
#define EOCDLEN             22
#define BUFSIZE       (MAXCOMLEN+EOCDLEN)
#define LOCAL_MAGIC   "PK\3\4"
#define CENTRAL_MAGIC "PK\1\2"
#define EOCD_MAGIC    "PK\5\6"
#define DATADES_MAGIC "PK\7\8"
#define CDENTRYSIZE        36

#undef NEVER
#ifdef NEVER
#define READ2(a)      (*((a)++)+(*((a)++))*256)
#define READ4(a)      (READ2(a)+READ2(a)*65536)
#endif

char * zip_err_str[]={
    "no error",
    "multi-disk zip-files not supported"
};



int
read2(unsigned char **a)
{
    int ret;

    ret = (*a)[0]+(*a)[1]*256;
    *a += 2;

    return ret;
}



int
read4(unsigned char **a)
{
    int ret;

    ret = (*a)[0]+(*a)[1]*256+(*a)[2]*65536+(*a)[3]*16777216;
    *a += 4;

    return ret;
}



char *
readstr(unsigned char **buf, int len)
{
    char *r;

    r = memdup(*buf, len);
    *buf += len;

    return r;
}



char *
readfpstr(FILE *fp, int len)
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
    unsigned char *buf, *match;
    int a, i, buflen, best;
    struct zf *cdir, *cdirnew;
    long len;

    if ((fp=fopen(fn, "rb"))==NULL)
	return NULL;

    clearerr(fp);
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    i = fseek(fp, -(len < BUFSIZE ? len : BUFSIZE), SEEK_END);
    if (i == -1 && errno != EFBIG) {
	/* seek before start of file on my machine */
	fclose(fp);
	return NULL;
    }

    buf = (unsigned char *)xmalloc(BUFSIZE);

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
	/* to avoid finding the same match all over again */
	/* XXX: better way? */
	match++;
	if ((cdirnew=readcdir(fp, buf, match-1, buflen)) == NULL)
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

    if (best == -2)
      best = checkcons(cdir, fp);

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



int
zip_close(struct zf *zf)
{
    int i, count, tfd;
    char *temp;
    FILE *tfp;

    if (zf->changes == 0)
	return zf_free(zf);

    temp = (char *)xmalloc(strlen(zf->zn)+8);
    sprintf(temp, "%s.XXXXXX", zf->zn);

    tfd = mkstemp(temp);

    if ((tfp=fdopen(tfd, "r+b")) == NULL) {
	free(temp);
	/* XXX: close file, free struct? */
	return -1;
    }
        
    count = 0;
    if (zf->entry) {
	for (i=0; i<zf->nentry; i++) {
	    switch (zf->entry[i].state) {
	    case Z_UNCHANGED:
		/* XXX: syntax? */
		/* copy_verbose(zf->fp, zf->entry[i], tfp); */
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

    if ((fclose(tfp)==0) && (fclose(zf->zp)==0)) {
	remove(zf->zn);
	rename(temp, zf->zn);
    }

    free(temp);
    zf_free(zf);

    return 0;
}



struct zf *
readcdir(FILE *fp, unsigned char *buf, unsigned char *eocd, int buflen)
{
    struct zf *zf;
    unsigned char *cdp;
    int i, comlen, readp;

    comlen = buf + buflen - eocd - EOCDLEN;
    if (comlen < 0) {
	/* not enough bytes left for comment */
	return NULL;
    }

    /* check for end-of-central-dir magic */
    if (memcmp(eocd, EOCD_MAGIC, 4) != 0)
	return NULL;

    if (memcmp(eocd+4, "\0\0\0\0", 4) != 0) {
	zip_err = ZERR_MULTIDISK;
	return NULL;
    }

    zf = zf_new();

    cdp = eocd + 8;
    /* number of cdir-entries on this disk */
    i = read2(&cdp);
    printf("i = %d\n", i);
    /* number of cdir-entries */
    zf->nentry = read2(&cdp);
    printf("nentry = %d\n", zf->nentry);
    zf->cd_size = read4(&cdp);
    printf("cd_size = %d\n", zf->cd_size);
    zf->cd_offset = read4(&cdp);
    printf("cd_offset = %d\n", zf->cd_offset);
    zf->com_size = read2(&cdp);
    printf("com_size = %d (%d)\n", zf->com_size, comlen);
    zf->entry = NULL;

    if ((zf->com_size != comlen) || (zf->nentry != i)) {
	/* comment size wrong -- too few or too many left after central dir */
	/* or number of cdir-entries on this disk != number of cdir-entries */
	zf_free(zf);
	return NULL;
    }

    zf->com = (unsigned char *)memdup(eocd+EOCDLEN, zf->com_size);

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
	if ((readcdentry(fp, zf->entry+i, &cdp, eocd-cdp, readp)) < 0) {
	    /* i entries have already been filled, tell zf_free
	       how many to free */
	    zf_free(zf);
	    return NULL;
	}
    }
    
    return zf;
}



int
readcdentry(FILE *fp, struct zf_entry *zfe, unsigned char **cdpp, 
	    int left, int readp)
{
    unsigned char buf[CDENTRYSIZE];
    unsigned char *cur;
    
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
    zfe->version_made = read2(&cur);
    zfe->version_need = read2(&cur);
    zfe->bitflags = read2(&cur);
    zfe->comp_meth = read2(&cur);
    zfe->lmtime = read2(&cur);
    zfe->lmdate = read2(&cur);

    zfe->crc = read4(&cur);
    zfe->comp_size = read4(&cur);
    zfe->uncomp_size = read4(&cur);
    
    zfe->fnlen = read2(&cur);
    zfe->eflen = read2(&cur);
    zfe->fcomlen = read2(&cur);
    zfe->disknrstart = read2(&cur);
    zfe->intatt = read2(&cur);

    zfe->extatt = read4(&cur);
    zfe->local_offset = read4(&cur);

    if (left < CDENTRYSIZE+zfe->fnlen+zfe->eflen+zfe->fcomlen) {
	if (readp) {
	    if (zfe->fnlen)
		zfe->fn = readfpstr(fp, zfe->fnlen);
	    if (zfe->eflen)
		zfe->ef = readfpstr(fp, zfe->eflen);
	    if (zfe->fcomlen)
		zfe->fcom = readfpstr(fp, zfe->fcomlen);
	}
	else {
	    /* can't get more bytes if not allowed to read */
	    return -1;
	}
    }
    else {
        if (zfe->fnlen)
	    zfe->fn = readstr(&cur, zfe->fnlen);
        if (zfe->eflen)
	    zfe->ef = readstr(&cur, zfe->eflen);
        if (zfe->fcomlen)
	    zfe->fcom = readstr(&cur, zfe->fcomlen);
    }
    if (!readp)
      *cdpp = cur;

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

    zf->zn = NULL;
    zf->zp = NULL;
    zf->nentry = zf->com_size = zf->changes = 0;
    zf->cd_size = zf->cd_offset = 0;
    zf->com = NULL;
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
