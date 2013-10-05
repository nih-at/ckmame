#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dlfcn.h>

static int inited = 0;
static size_t count = 0;
static size_t max_write = 0;
static size_t(*real_fwrite)(const void *ptr, size_t size, size_t nmemb, FILE * stream) = NULL;

static FILE *log;
static const char *myname = NULL;

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE * stream)
{
    size_t ret;

    if (!inited) {
	char *foo;
	myname = getprogname();
        log = fopen("/tmp/fwrite.log", "a");
	if (!myname)
	    myname = "(unknown)";
	if ((foo=getenv("FWRITE_MAX_WRITE")) != NULL)
	    max_write = strtoul(foo, NULL, 0);
	fprintf(log, "%s: max_write set to %lu\n", myname, max_write);
	real_fwrite = dlsym(RTLD_NEXT, "fwrite");
	if (!real_fwrite)
	    abort();
	inited = 1;
    }
 
    if (count + size*nmemb > max_write) {
	fprintf(log, "%s: returned ENOSPC\n", myname);
	errno = ENOSPC;
	return -1;
    }
    

    ret = real_fwrite(ptr, size, nmemb, stream);
    count += ret * size;

    fprintf(log, "%s: wrote %lu*%lu = %lu bytes, sum %lu\n", myname, ret, size, ret * size, count);
    return ret;
}
