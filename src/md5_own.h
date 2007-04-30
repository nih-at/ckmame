#ifndef MD5_H
#define MD5_H

/* $NiH: md5_own.h,v 1.1 2005/07/04 21:54:51 dillo Exp $ */

#include "myinttypes.h"

struct MD5Context {
	uint32_t buf[4];
	uint32_t bits[2];
	unsigned char in[64];
};

typedef struct MD5Context MD5_CTX;

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, const unsigned char *, unsigned);
void MD5Final(unsigned char [16], MD5_CTX *);

#endif /* !MD5_H */
