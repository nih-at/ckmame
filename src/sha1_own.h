#ifndef SHA1_OWN_H
#define SHA1_OWN_H

#include "myinttypes.h"

#define SHA_DATASIZE    64
#define SHA_DATALEN     16
#define SHA_DIGESTSIZE  20
#define SHA_DIGESTLEN    5

/* The structure for storing SHA info */

typedef struct sha_ctx {
  uint32_t digest[SHA_DIGESTLEN];  /* Message digest */
  uint32_t count_l, count_h;       /* 64-bit block count */
  unsigned char block[SHA_DATASIZE];     /* SHA data buffer */
  int index;                             /* index into buffer */
} SHA_CTX;

typedef struct sha_ctx SHA1_CTX;

void SHA1Init(SHA1_CTX *context);
void SHA1Update(SHA1_CTX *, const unsigned char *, unsigned int);
void SHA1Final(unsigned char [20], SHA1_CTX *);

#endif /* sha1_own.h */

