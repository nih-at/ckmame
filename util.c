#include "types.h"

enum state
romcmp(struct rom *r1, struct rom *r2)
{
    /* r1 is important */
    if (strcasecmp(r1->name, r2->name) == 0) {
	if (r1->size == r2->size) {
	    if (r1->crc == r2->crc)
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
