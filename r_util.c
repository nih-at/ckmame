#include <stddef.h>

#include "dbl.h"
#include "r.h"
#include "types.h"
#include "funcs.h"

#define BLKSIZE  1024

unsigned short
r__ushort(DBT *v)
{
    unsigned short s;

    if (v->size < 2)
	return 0;
    
    s = (((unsigned char *)v->data)[0] << 8)
	| (((unsigned char *)v->data)[1]);

    v->size -= 2;
    v->data += 2;

    return s;
}



unsigned long
r__ulong(DBT *v)
{
    unsigned long l;

    if (v->size < 4)
	return 0;
    
    l = (((unsigned char *)v->data)[0] << 24)
	| (((unsigned char *)v->data)[1] << 16)
	| (((unsigned char *)v->data)[2] << 8)
	| (((unsigned char *)v->data)[3]);

    v->size -= 4;
    v->data += 4;

    return l;
}



char *
r__string(DBT *v)
{
    char *s;
    int len;

    len = r__ushort(v);
    if (len == 0)
	return NULL;
    
    if (v->size < len)
	return NULL;

    s = (char *)xmalloc(len);
	memcpy(s, (unsigned char *)v->data, len);
    v->size -= len;
    v->data += len;

    return s;
}



void
r__pstring(DBT *v, char **sp)
{
    *sp = r__string(v);
}

int
r__array(DBT *v, void (*fn)(DBT *, void *), void **a, size_t size)
{
    int n;
    int i;
    void *ap;
    
    n = r__ulong(v);
    if (n == 0) {
	*a = NULL;
	return 0;
    }

    ap = xmalloc(n*size);
    
    for (i=0; i<n; i++)
	fn(v, ap+(size*i));

    *a = ap;
    return n;
}



/* XXX: doesn't belong in this file */

void
r__rom(DBT *v, struct rom *r)
{
    r->name = r__string(v);
    r->size = r__ulong(v);
    r->crc = r__ulong(v);
    r->where = r__ushort(v);
    r->state = 0;
}
