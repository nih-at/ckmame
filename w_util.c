#include <stddef.h>

#include "dbl.h"
#include "types.h"
#include "xmalloc.h"
#include "w.h"

#define BLKSIZE  1024

void
w__grow(DBT *v, int n)
{
    int size;
    
    size = (v->size + BLKSIZE-1) / BLKSIZE;
    size *= BLKSIZE;

    if (v->size+n >= size) {
	v->data = xrealloc(v->data, size+BLKSIZE);
    }
}



void
w__ushort(DBT *v, unsigned short s)
{
    w__grow(v, 2);
    ((unsigned char *)v->data)[v->size++] = (s>> 8) & 0xff;
    ((unsigned char *)v->data)[v->size++] = s & 0xff;
}



void
w__ulong(DBT *v, unsigned long l)
{
    w__grow(v, 4);
    ((unsigned char *)v->data)[v->size++] = (l>> 24) & 0xff;
    ((unsigned char *)v->data)[v->size++] = (l>> 16) & 0xff;
    ((unsigned char *)v->data)[v->size++] = (l>> 8) & 0xff;
    ((unsigned char *)v->data)[v->size++] = l & 0xff;
}



void
w__string(DBT *v, char *s)
{
    int len;

    if (s == NULL)
	len = 0;
    else
	len = strlen(s)+1;

    w__ushort(v, len);
    w__grow(v, len);
    if (s)
	memcpy(((unsigned char *)v->data)+v->size, s, len);
    v->size += len;
}



void
w__pstring(DBT *v, void *sp)
{
    w__string(v, *(char **)sp);
}



void
w__array(DBT *v, void (*fn)(DBT *, void *), void *a, size_t size, size_t n)
{
    int i;
    
    w__ulong(v, n);

    for (i=0; i<n; i++)
	fn(v, a+(size*i));
}



/* XXX: doesn't belong in this file */

void
w__rom(DBT *v, void *r)
{
    w__string(v, ((struct rom *)r)->name);
    w__string(v, ((struct rom *)r)->merge);
    w__ulong(v, ((struct rom *)r)->size);
    w__array(v, w__pstring, ((struct rom *)r)->altname, sizeof(char *),
	     ((struct rom *)r)->naltname);
    w__ulong(v, ((struct rom *)r)->crc);
    w__ushort(v, ((struct rom *)r)->where);
}
