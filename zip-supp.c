#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "types.h"
#include "error.h"
#include "dbl.h"
#include "funcs.h"
#include "xmalloc.h"

#define MAXFNLEN 1024
#define BUFSIZE 8192

int
findcrc(struct zfile *zip, int idx, int romsize, unsigned long wcrc)
{
    struct zip_file *zff;
    unsigned long crc;
    char buf[BUFSIZE];
    int n, left, offset;

    if ((zff = zip_fopen_index(zip->zf, idx)) == NULL)
	return -1;

    offset = 0;
    while (offset+romsize <= zip->rom[idx].size) {
	left = BUFSIZE;
	crc = crc32(0, NULL, 0);
	n = romsize;
	while (n > 0) {
	    if (left > n)
		left = n;
	    if (zip_fread(zff, buf, left) != left) {
		/* XXX: error */
		zip_fclose(zff);
		return -1;
	    }
	    crc = crc32(crc, buf, left);
	    n -= left;
	}

	if (crc == wcrc)
	    break;

	offset += romsize;
    }

    if (zip_fclose(zff))
	return -1;
    
    if (crc == wcrc)
	return offset;
	    
    return -1;
}




int
zfile_free(struct zfile *zip)
{
    int i, ret;

    if (zip == NULL)
	return 0;
    
    free(zip->name);
    for (i=0; i<zip->nrom; i++)
	free(zip->rom[i].name);

    if (zip->nrom)
	free(zip->rom);

    if (zip->zf)
	ret = zip_close(zip->zf);

    free(zip);

    return ret;
}



int
readinfosfromzip (struct zfile *z)
{
    struct zip *zf;
    int i;

    z->nrom = 0;
    z->rom = NULL;
    z->zf = NULL;

    if ((zf=zip_open(z->name, 0))==NULL)
	return -1;

    z->rom = (struct rom *)xmalloc(sizeof(struct rom)*(zf->nentry));
    z->nrom = zf->nentry;
    z->zf = zf;

    for (i=0; i<zf->nentry; i++) {
	z->rom[i].name = xstrdup(zf->entry[i].fn);
	z->rom[i].size = zf->entry[i].uncomp_size;
	z->rom[i].crc = zf->entry[i].crc;
	z->rom[i].state = ROM_0;
    }	

    return i;
}
