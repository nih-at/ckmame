#include <stdlib.h>

#include "types.h"
#include "error.h"

void *xmalloc(size_t size);

enum state
romcmp(struct rom *r1, struct rom *r2)
{
    /* r1 is important */
    if (strcasecmp(r1->name, r2->name) == 0) {
	if (r1->size == r2->size) {
	    if (r1->crc == r2->crc || r2->crc == 0 || r1->crc == 0)
		return ROM_OK;
	    else
		return ROM_CRCERR;
	}
	else if (r1->size > r2->size)
	    return ROM_LONG;
	else
	    return ROM_SHORT;
    }
    else if (r1->size == r2->size && r1->crc == r2->crc)
	return ROM_NAMERR;
    else
	return ROM_UNKNOWN;
}



char *
findzip(char *name)
{
    char *s;

    s = xmalloc(strlen(name)+10);

    sprintf(s, "roms/%s.zip", name);

    return s;
}
