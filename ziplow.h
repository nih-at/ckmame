#ifndef _HAD_ZIPLOW_H
#define _HAD_ZIPLOW_H

#include "zip.h"

struct zf *readcdir(FILE *fp, unsigned char *buf, unsigned char *eocd, 
		    int buflen);
struct zf *zf_new(void);
int zf_free(struct zf *zf);
struct zf *zip_open(char *fn);
int zip_close(struct zf *zf);
char *readstr(unsigned char **buf, int len);
char *readfpstr(FILE *fp, int len);
int read2(unsigned char **a);
int read4(unsigned char **a);
int readcdentry(FILE *fp, struct zf_entry *zfe, unsigned char **cdpp, 
		int left, int readp);
int checkcons(struct zf *zf, FILE *fp);

#endif /* _HAD_ZIPLOW_H */
