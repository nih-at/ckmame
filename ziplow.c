#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "ziplow.h"
#include "zip.h"
#include "error.h"

#include "util.h"
#include "xmalloc.h"

#define MAXCOMLEN        65536
#define EOCDLEN             22
#define BUFSIZE       (MAXCOMLEN+EOCDLEN)
#define LOCAL_MAGIC   "PK\3\4"
#define CENTRAL_MAGIC "PK\1\2"
#define EOCD_MAGIC    "PK\5\6"
#define DATADES_MAGIC "PK\7\8"
#define CDENTRYSIZE         46
#define LENTRYSIZE          30

#define ALLOC_SIZE         100

char * zip_err_str[]={
    "no error",
    "multi-disk zip-files not supported",
    "replacing zipfile with tempfile failed",
    "closing zipfile failed",
    "seek error",
    "read error",
    "write error"
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
readstr(unsigned char **buf, int len, int nullp)
{
    char *r;

    r = (char *)xmalloc(nullp?len+1:len);
    memcpy(r, *buf, len);
    *buf += len;

    if (nullp)
	r[len] = 0;

    return r;
}



void
write2(FILE *fp, int i)
{
    fputc(i&0xff, fp);
    fputc((i>>8)&0xff, fp);

    return;
}



void
write4(FILE *fp, int i)
{
    fputc(i&0xff, fp);
    fputc((i>>8)&0xff, fp);
    fputc((i>>16)&0xff, fp);
    fputc((i>>24)&0xff, fp);
    
    return;
}



void
writestr(FILE *fp, char *str, int len)
{
    fprintf(fp, "%.*s", len, str);
    return;
}



char *
readfpstr(FILE *fp, int len, int nullp)
{
    char *r;

    r = (char *)xmalloc(nullp?len+1:len);
    if (fread(r, 1, len, fp)<len) {
	free(r);
	return NULL;
    }

    if (nullp)
	r[len] = 0;
    
    return r;
}



/* zip_open:
   Tries to open the file 'fn' as a zipfile. If checkp, also does some
   consistency checks (comparing local headers to central directory
   entries). Returns a zipfile struct, or NULL if unsuccessful. */

struct zf *
zip_open(char *fn, int checkp)
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
    
    best = -1;
    cdir = NULL;
    match = buf;
    while ((match=memmem(match, buflen-(match-buf)-18, EOCD_MAGIC, 4))!=NULL) {
	/* found match -- check, if good */
	/* to avoid finding the same match all over again */
	match++;
	if ((cdirnew=readcdir(fp, buf, match-1, buflen)) == NULL)
	    continue;	    

	if (cdir) {
	    if (best <= 0)
		best = checkcons(fp, cdir);
	    a = checkcons(fp, cdirnew);
	    if (best < a) {
		zf_free(cdir);
		cdir = cdirnew;
		best = a;
	    }
	    else
		zf_free(cdirnew);
	}
	else {
	    cdir = cdirnew;
	    if (checkp)
		best = checkcons(fp, cdir);
	    else
		best = 0;
	}
	cdirnew = NULL;
    }

    if (best < 0) {
	/* no consistent eocd found */
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



/* zip_close:
   Tries to commit all changes and close the zipfile; if it fails,
   zip_err (and stderr) are set and *zf is unchanged, except for
   problems in zf_free. */ 

int
zip_close(struct zf *zf)
{
    int i, count, tfd;
    char *temp, *keep;
    FILE *tfp;
    struct zf *tzf;

    if (zf->changes == 0)
	return zf_free(zf);

    /* look if there really are any changes */
    count = 0;
    for (i=0; i<zf->nentry; i++) {
	if (zf->entry[i].state != Z_UNCHANGED) {
	    count = 1;
	    break;
	}
    }

    /* no changes, all has been unchanged */
    if (count == 0)
	return zf_free(zf);
    
    temp = (char *)xmalloc(strlen(zf->zn)+8);
    sprintf(temp, "%s.XXXXXX", zf->zn);

    tfd = mkstemp(temp);

    if ((tfp=fdopen(tfd, "r+b")) == NULL) {
	free(temp);
	close(tfd);
	return -1;
    }

    tzf = zf_new();
    tzf->zp = tfp;
    tzf->zn = temp;
    tzf->nentry = 0;
    tzf->comlen = zf->comlen;
    tzf->cd_size = tzf->cd_offset = 0;
    tzf->com = (unsigned char *)memdup(zf->com, zf->comlen);
    tzf->entry = (struct zf_entry *)xmalloc(sizeof(struct
						   zf_entry)*ALLOC_SIZE);
    tzf->nentry_alloc = ALLOC_SIZE;
    
    count = 0;
    if (zf->entry) {
	for (i=0; i<zf->nentry; i++) {
	    switch (zf->entry[i].state) {
	    case Z_UNCHANGED:
		zip_entry_copy(tzf, zf, i);
		break;
	    case Z_DELETED:
		break;
	    case Z_REPLACED:
		/* fallthrough */
	    case Z_ADDED:
		if (zf->entry[i].ch_data_zf) {
		    /* keep original ch_name in src struct zf */
		    keep = zf->entry[i].ch_data_zf->entry[zf->entry[i].ch_data_zf_fileno].ch_name;
		    zf->entry[i].ch_data_zf->entry[zf->entry[i].ch_data_zf_fileno].ch_name =
			zf->entry[i].ch_name;
		    zip_entry_copy(tzf, zf->entry[i].ch_data_zf,
				   zf->entry[i].ch_data_zf_fileno);
		    zf->entry[i].ch_data_zf->entry[zf->entry[i].ch_data_zf_fileno].ch_name =
			keep;
		} else if (zf->entry[i].ch_data_buf) {
#if 0
		    zip_entry_add(tzf, zf->entry[i], 0);
#endif 0
		} else if (zf->entry[i].ch_data_fp) {
#if 0
		    zip_entry_add(tzf, zf->entry[i], 0);
#endif 0
		} else {
		    /* XXX: ? */
		    myerror(ERRFILE, "Z_ADDED: no data");
		    break;
		}
		free(tzf->entry[tzf->nentry-1].fn);
		tzf->entry[tzf->nentry-1].fn =
		    xstrdup(zf->entry[i].ch_name);
		break;
	    case Z_RENAMED:
		zip_entry_copy(tzf, zf, i);
		free(tzf->entry[tzf->nentry-1].fn);
		tzf->entry[tzf->nentry-1].fn =
		    xstrdup(zf->entry[i].ch_name);
		break;
	    default:
		/* can't happen */
		break;
	    }
	}
    }

    writecdir(tzf);
    
    if ((fclose(tzf->zp)==0) && (fclose(zf->zp)==0)) {
	if (rename(tzf->zn, zf->zn) != 0) {
	    zip_err = ZERR_RENAME;
	    zf_free(tzf);
	    return -1;
	}
    }

    free(temp);
    zf_free(zf);

    return 0;
}



int
zip_entry_copy(struct zf *dest, struct zf *src, int entry_no)
{
    char buf[BUFSIZE];
    unsigned int len, remainder;
    unsigned char *null;
    struct zf_entry tempzfe;

    null = NULL;

    if (dest->nentry == dest->nentry_alloc) {
	dest->nentry_alloc += ALLOC_SIZE;
	dest->entry = (struct zf_entry *)xrealloc(dest->entry,
		       sizeof(struct zf_entry)*dest->nentry_alloc);
    }

    /* copy values from original zf_entry */
    dest->entry[dest->nentry].version_made = src->entry[entry_no].version_made;
    dest->entry[dest->nentry].version_need = src->entry[entry_no].version_need;
    dest->entry[dest->nentry].bitflags = src->entry[entry_no].bitflags;
    dest->entry[dest->nentry].comp_meth = src->entry[entry_no].comp_meth;
    dest->entry[dest->nentry].lmtime = src->entry[entry_no].lmtime;
    dest->entry[dest->nentry].lmdate = src->entry[entry_no].lmdate;
    dest->entry[dest->nentry].fcomlen = src->entry[entry_no].fcomlen;
    dest->entry[dest->nentry].eflen = src->entry[entry_no].eflen;
    dest->entry[dest->nentry].disknrstart = src->entry[entry_no].disknrstart;
    dest->entry[dest->nentry].intatt = src->entry[entry_no].intatt;
    dest->entry[dest->nentry].crc = src->entry[entry_no].crc;
    dest->entry[dest->nentry].comp_size = src->entry[entry_no].comp_size;
    dest->entry[dest->nentry].uncomp_size = src->entry[entry_no].uncomp_size;
    dest->entry[dest->nentry].extatt = src->entry[entry_no].extatt;
    dest->entry[dest->nentry].ef = (char *)memdup(src->entry[entry_no].ef,
				    src->entry[entry_no].eflen);
    dest->entry[dest->nentry].fcom = (char *)memdup(src->entry[entry_no].fcom,
				      src->entry[entry_no].fcomlen);

    dest->entry[dest->nentry].local_offset = ftell(dest->zp);

    if (src->entry[entry_no].ch_name) {
	dest->entry[dest->nentry].fn = xstrdup(src->entry[entry_no].ch_name);
	dest->entry[dest->nentry].fnlen =
	    strlen(src->entry[entry_no].ch_name);
    } else {
	dest->entry[dest->nentry].fn = xstrdup(src->entry[entry_no].fn);
	dest->entry[dest->nentry].fnlen = src->entry[entry_no].fnlen;
    }
    
    dest->entry[dest->nentry].state = Z_UNCHANGED;
    dest->entry[dest->nentry].ch_name = NULL;
    dest->entry[dest->nentry].ch_data_fp = NULL;
    dest->entry[dest->nentry].ch_data_buf = NULL;
    dest->entry[dest->nentry].ch_data_zf = NULL;
    dest->entry[dest->nentry].ch_data_offset = 0;
    dest->entry[dest->nentry].ch_data_len = 0;
    dest->entry[dest->nentry].ch_data_zf_fileno = 0;

    if (fseek(src->zp, src->entry[entry_no].local_offset, SEEK_SET) != 0) {
	zip_err = ZERR_SEEK;
	return -1;
    }

    if (readcdentry(src->zp, &tempzfe, &null, 0, 1, 1) != 0) {
	zip_err = ZERR_READ;
	return -1;
    }
    if (src->entry[entry_no].ch_name) {
	free(tempzfe.fn);
	tempzfe.fn = xstrdup(src->entry[entry_no].ch_name);
	tempzfe.fnlen = strlen(src->entry[entry_no].ch_name);
    }
    if (writecdentry(dest->zp, &tempzfe, 1) != 0) {
	zip_err = ZERR_WRITE;
	return -1;
    }
    
    remainder = src->entry[entry_no].comp_size;
    len = BUFSIZE;
    while (remainder) {
	if (len < remainder)
	    len = remainder;
	if (fread(buf, 1, len, src->zp)!=len) {
	    zip_err = ZERR_READ;
	    return -1;
	}
	if (fwrite(buf, 1, len, dest->zp)!=len) {
	    zip_err = ZERR_WRITE;
	    return -1;
	}
	remainder -= len;
    }

    dest->nentry++;
    return 0;
}



int
writecdir(struct zf *zfp)
{
    int i;
    long cd_offset, cd_size;

    cd_offset = ftell(zfp->zp);

    for (i=0; i<zfp->nentry; i++) {
	if (writecdentry(zfp->zp, zfp->entry+i, 0) != 0) {
	    zip_err = ZERR_WRITE;
	    return -1;
	}
    }

    cd_size = ftell(zfp->zp) - cd_offset;
    
    clearerr(zfp->zp);
    fprintf(zfp->zp, EOCD_MAGIC);
    fprintf(zfp->zp, "%c%c%c%c", 0, 0, 0, 0);
    fprintf(zfp->zp, "%c%c%c%c", (zfp->nentry>>8)&0xff, zfp->nentry&0xff,
	    (zfp->nentry>>8)&0xff, zfp->nentry&0xff);
    fprintf(zfp->zp, "%c%c%c%c", (int)(cd_size>>24)&0xff, 
	    (int)(cd_size>>16)&0xff, (int)(cd_size>>8)&0xff, 
	    (int)cd_size&0xff);
    fprintf(zfp->zp, "%c%c%c%c", (int)(cd_offset>>24)&0xff, 
	    (int)(cd_offset>>16)&0xff, (int)(cd_offset>>8)&0xff, 
	    (int)cd_offset&0xff);
    fprintf(zfp->zp, "%c%c", (zfp->comlen>>8)&0xff, zfp->comlen&0xff);
    fprintf(zfp->zp, "%.*s", zfp->comlen, zfp->com);

    /* XXX: incomplete */
    
    return 0;
}



/* readcdir:
   tries to find a valid end-of-central-directory at the beginning of
   buf, and then the corresponding central directory entries.
   Returns a zipfile struct which contains the central directory 
   entries, or NULL if unsuccessful. */

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
    /* number of cdir-entries */
    zf->nentry = zf->nentry_alloc = read2(&cdp);
    zf->cd_size = read4(&cdp);
    zf->cd_offset = read4(&cdp);
    zf->comlen = read2(&cdp);
    zf->entry = NULL;

    if ((zf->comlen != comlen) || (zf->nentry != i)) {
	/* comment size wrong -- too few or too many left after central dir */
	/* or number of cdir-entries on this disk != number of cdir-entries */
	zf_free(zf);
	return NULL;
    }

    zf->com = (unsigned char *)memdup(eocd+EOCDLEN, zf->comlen);

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
	fseek(fp, -(zf->cd_size+zf->comlen+EOCDLEN), SEEK_END);
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
	zf->entry[i].ch_name = NULL;
	zf->entry[i].ch_data_fp = NULL;
	zf->entry[i].ch_data_buf = NULL;
	zf->entry[i].ch_data_zf = NULL;
    }
    
    for (i=0; i<zf->nentry; i++) {
	if ((readcdentry(fp, zf->entry+i, &cdp, eocd-cdp, readp, 0)) < 0) {
	    /* i entries have already been filled, tell zf_free
	       how many to free */
	    zf_free(zf);
	    return NULL;
	}
    }
    
    return zf;
}



/* readcdentry:
   fills the zipfile entry zfe with data from the buffer *cdpp, not reading
   more than 'left' bytes from it; if readp != 0, it also reads more data
   from fp, if necessary. If localp != 0, it reads a local header instead
   of a central directory entry. Returns 0 if successful, -1 if not,
   advancing *cdpp for each byte read. */

int
readcdentry(FILE *fp, struct zf_entry *zfe, unsigned char **cdpp, 
	    int left, int readp, int localp)
{
    unsigned char buf[CDENTRYSIZE];
    unsigned char *cur;
    int size;

    if (localp)
	size = LENTRYSIZE;
    else
	size = CDENTRYSIZE;
    
    if (readp) {
	/* read entry from disk */
	if ((fread(buf, 1, size, fp)<size))
	    return -1;
	left = size;
	cur = buf;
    }
    else {
	cur = *cdpp;
	if (left < size)
	    return -1;
    }

    if (localp) {
	if (memcmp(cur, LOCAL_MAGIC, 4)!=0)
	    return -1;
    }
    else
	if (memcmp(cur, CENTRAL_MAGIC, 4)!=0)
	    return -1;

    cur += 4;

    /* convert buffercontents to zf_entry */
    if (!localp)
	zfe->version_made = read2(&cur);
    else
	zfe->version_made = 0;
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
    if (!localp) {
	zfe->fcomlen = read2(&cur);
	zfe->disknrstart = read2(&cur);
	zfe->intatt = read2(&cur);

	zfe->extatt = read4(&cur);
	zfe->local_offset = read4(&cur);
    }
    else {
	zfe->fcomlen = zfe->disknrstart = zfe->intatt = 0;
	zfe->extatt = zfe->local_offset = 0;
    }
    
    if (left < CDENTRYSIZE+zfe->fnlen+zfe->eflen+zfe->fcomlen) {
	if (readp) {
	    if (zfe->fnlen)
		zfe->fn = readfpstr(fp, zfe->fnlen, 1);
	    if (zfe->eflen)
		zfe->ef = readfpstr(fp, zfe->eflen, 0);
	    /* XXX: really null-terminate comment? */
	    if (zfe->fcomlen)
		zfe->fcom = readfpstr(fp, zfe->fcomlen, 1);
	}
	else {
	    /* can't get more bytes if not allowed to read */
	    return -1;
	}
    }
    else {
        if (zfe->fnlen)
	    zfe->fn = readstr(&cur, zfe->fnlen, 1);
        if (zfe->eflen)
	    zfe->ef = readstr(&cur, zfe->eflen, 0);
        if (zfe->fcomlen)
	    zfe->fcom = readstr(&cur, zfe->fcomlen, 1);
    }
    if (!readp)
      *cdpp = cur;

    zfe->ch_name = NULL;
    zfe->ch_data_fp = NULL;
    zfe->ch_data_buf = NULL;
    zfe->ch_data_offset = 0;
    zfe->ch_data_len = 0;

    return 0;
}


/* writecdentry:
   if localp, writes local header for zfe to zf->zp,
   else write central directory entry for zfe to zf->zp.
   if after writing ferror(fp), return -1, else return 0.*/
   
int
writecdentry(FILE *fp, struct zf_entry *zfe, int localp)
{
    fprintf(fp, "%s", localp?LOCAL_MAGIC:CENTRAL_MAGIC);
    
    if (!localp)
	write2(fp, zfe->version_made);
    write2(fp, zfe->version_need);
    write2(fp, zfe->bitflags);
    write2(fp, zfe->comp_meth);
    write2(fp, zfe->lmtime);
    write2(fp, zfe->lmdate);

    write4(fp, zfe->crc);
    write4(fp, zfe->comp_size);
    write4(fp, zfe->uncomp_size);
    
    write2(fp, zfe->fnlen);
    write2(fp, zfe->eflen);
    if (!localp) {
	write2(fp, zfe->fcomlen);
	write2(fp, zfe->disknrstart);
	write2(fp, zfe->intatt);

	write4(fp, zfe->extatt);
	write4(fp, zfe->local_offset);
    }
    
    if (zfe->fnlen)
	writestr(fp, zfe->fn, zfe->fnlen);
    if (zfe->eflen)
	writestr(fp, zfe->ef, zfe->eflen);
    if (zfe->fcomlen)
	writestr(fp, zfe->fcom, zfe->fcomlen);

    if (ferror(fp))
	return -1;
    
    return 0;
}



/* checkcons:
   Checks the consistency of the central directory by comparing central
   directory entries with local headers and checking for plausible
   file and header offsets. Returns -1 if not plausible, else the
   difference between the lowest and the highest fileposition reached */

int
checkcons(FILE *fp, struct zf *zf)
{
    int min, max, i, j;
    struct zf_entry temp;
    unsigned char *buf;

    buf = NULL;

    if (zf->nentry) {
	max = zf->entry[0].local_offset;
	min = zf->entry[0].local_offset;
    }

    for (i=0; i<zf->nentry; i++) {
	if (zf->entry[i].local_offset < min)
	    min = zf->entry[i].local_offset;
	if (min < 0)
	    return -1;

	j = zf->entry[i].local_offset + zf->entry[i].comp_size
	    + zf->entry[i].fnlen + zf->entry[i].eflen
	    + zf->entry[i].fcomlen + LENTRYSIZE;
	if (j > max)
	    max = j;
	if (max > zf->cd_offset)
	    return -1;

	if (fseek(fp, zf->entry[i].local_offset, SEEK_SET) != 0) {
	    zip_err = ZERR_SEEK;
	    return -1;
	}
	readcdentry(fp, &temp, &buf, 0, 1, 1);
	if (headercomp(zf->entry+i, 0, &temp, 1) != 0)
	    return -1;
    }

    return max - min;
}



/* headercomp:
   compares two headers h1 and h2; if they are local headers, set
   local1p or local2p respectively to 1, else 0. Return 0 if they
   are identical, -1 if not. */

int
headercomp(struct zf_entry *h1, int local1p, struct zf_entry *h2,
	   int local2p)
{
    if ((h1->version_need != h2->version_need)
	|| (h1->bitflags != h2->bitflags)
	|| (h1->comp_meth != h2->comp_meth)
	|| (h1->lmtime != h2->lmtime)
	|| (h1->lmdate != h2->lmdate)
	|| (h1->fnlen != h2->fnlen)
	|| (h1->crc != h2->crc)
	|| (h1->comp_size != h2->comp_size)
	|| (h1->uncomp_size != h2->uncomp_size)
	|| (h1->fnlen && memcmp(h1->fn, h2->fn, h1->fnlen)))
	return -1;

    /* if they are different type, nothing more to check */
    if (local1p != local2p)
	return 0;

    if ((h1->version_made != h2->version_made)
	|| (h1->disknrstart != h2->disknrstart)
	|| (h1->intatt != h2->intatt)
	|| (h1->extatt != h2->extatt)
	|| (h1->local_offset != h2->local_offset)
	|| (h1->eflen != h2->eflen)
	|| (h1->eflen && memcmp(h1->fn, h2->fn, h1->fnlen))
	|| (h1->fcomlen != h2->fcomlen)
	|| (h1->fcomlen && memcmp(h1->fcom, h2->fcom, h1->fcomlen))) {
	return -1;
    }

    return 0;
}



/* zf_new:
   creates a new zipfile struct, and sets the contents to zero; returns
   the new struct. */

struct zf *
zf_new(void)
{
    struct zf *zf;

    zf = (struct zf *)xmalloc(sizeof(struct zf));

    zf->zn = NULL;
    zf->zp = NULL;
    zf->comlen = zf->changes = 0;
    zf->nentry = zf->nentry_alloc = zf->cd_size = zf->cd_offset = 0;
    zf->com = NULL;
    zf->entry = NULL;
    zf->unz_zst = NULL;
    zf->unz_in = NULL;
    
    return zf;
}



/* zf_free:
   frees the space allocated to a zipfile struct, and closes the
   corresponding file. Returns 0 if successful, the error returned
   by fclose if not. */

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
	    if (zf->entry[i].ch_name)
		free(zf->entry[i].ch_name);
	    if (zf->entry[i].ch_data_buf)
		free(zf->entry[i].ch_data_buf);
	    if (zf->entry[i].ch_data_fp)
		(void)fclose(zf->entry[i].ch_data_fp);
	}
	free (zf->entry);
    }

    if (zf->unz_zst)
	free(zf->unz_zst);

    if (zf->unz_in)
	free(zf->unz_in);

    free(zf);

    if (ret)
	zip_err = ZERR_CLOSE;
    
    return ret;
}



int
zf_read(struct zf *zf, int fileno, char *outbuf, int toread)
{
    int i, len;

    if (fileno != zf->unz_last) {
	/* go to start of actual compressed data */
	fseek (zf->zp, zf->entry[i].local_offset+LENTRYSIZE
	       +zf->entry[i].fnlen+zf->entry[i].fcomlen
	       +zf->entry[i].eflen, SEEK_SET);
	
	/* remove remainder of last stream (if any) */
	free(zf->unz_in);
	free(zf->unz_zst);

	zf->unz_zst = (z_stream *)xmalloc(sizeof(z_stream));
	zf->unz_in = (char *)xmalloc(BUFSIZE);
	zf->unz_zst->zalloc = Z_NULL;
	zf->unz_zst->zfree = Z_NULL;
	zf->unz_zst->opaque = NULL;

	seterrinfo(zf->entry[i].fn, zf->zn);

	len = fread (zf->unz_in, 1, BUFSIZE, zf->zp);
	if (len <= 0) {
	    myerror (ERRSTR, "read error");
	    return -1;
	}
	
	zf->unz_zst->next_in = zf->unz_in;
	zf->unz_zst->avail_in = len;
	if (inflateInit(zf->unz_zst) != Z_OK) {
	    myerror(ERRFILE, zf->unz_zst->msg);
	    return -1;
	}
    }


    /* XXX: main part missing */

    return 0;
}
