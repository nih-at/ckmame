void w__grow(DBT *v, int n);
void w__ushort(DBT *v, unsigned short s);
void w__ulong(DBT *v, unsigned long l);
void w__string(DBT *v, char *s);
void w__pstring(DBT *v, char **sp);
void w__array(DBT *v, void (*fn)(DBT *, void *), void *a, size_t size, size_t n);

#include "types.h"
void w__rom(DBT *v, struct rom *r);
