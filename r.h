unsigned short r__ushort(DBT *v);
unsigned long r__ulong(DBT *v);
char *r__string(DBT *v);
void r__pstring(DBT *v, void *sp);
int r__array(DBT *v, void (*fn)(DBT *, void *), void **a, size_t size);

#include "types.h"

void r__rom(DBT *v, void *r);
