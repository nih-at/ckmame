#ifndef _HAD_ZIPLOW_H
#define _HAD_ZIPLOW_H

#include "zip.h"

struct zf *readcdir(FILE *fp, unsigned char *buf, unsigned char *eocd, 
		    int buflen);
struct zf *zf_new(void);
int zf_free(struct zf *zf);
struct zf *zip_open(char *fn, int checkp);
int zip_close(struct zf *zf);
char *readstr(unsigned char **buf, int len, int nullp);
char *readfpstr(FILE *fp, int len, int nullp);
int read2(unsigned char **a);
int read4(unsigned char **a);
int readcdentry(FILE *fp, struct zf_entry *zfe, unsigned char **cdpp, 
		int left, int readp, int localp);
int checkcons(FILE *fp, struct zf *zf);
int headercomp(struct zf_entry *h1, int local1p, struct zf_entry *h2,
	       int local2p);
int zip_entry_copy(struct zf *dest, struct zf *src, int entry_no);

#endif /* _HAD_ZIPLOW_H */
