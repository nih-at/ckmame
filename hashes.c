#include <string.h>

#include "types.h"
#include "romutil.h"

/*
  return 0 on match, 1 on mismatch, -1 on no common types
  XXX: use named constants
*/

int
hashes_cmp(const struct hashes *h1, const struct hashes *h2)
{
    if ((h1->types & h2->types) == 0)
	return -1;

    if (h1->types & h2->types & GOT_CRC)
	if (h1->crc != h2->crc)
	    return 1;

    if (h1->types & h2->types & GOT_MD5)
	if (memcmp(h1->md5, h2->md5, sizeof(h1->md5)) != 0)
	    return 1;

    if (h1->types & h2->types & GOT_SHA1)
	if (memcmp(h1->sha1, h2->sha1, sizeof(h1->sha1)) != 0)
	    return 1;

    return 0;
}



void
hashes_init(struct hashes *h)
{
    h->types = 0;
    h->crc = 0;
    memset(h->md5, 0, sizeof(h->md5));
    memset(h->sha1, 0, sizeof(h->sha1));
}
