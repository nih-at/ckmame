#include "zip.h"
#include "error.h"
#include "ziplow.h"
#include "zff.h"

char *prg;
#define BUFSIZE 65536

int
main(int argc, char *argv[])
{
    int i;
    struct zf *zf;
    struct zf_file *zff;
    char buf[BUFSIZE];
    
    prg = argv[0];
    
    if (argc != 2) {
	myerror(ERRDEF, "call with one option: the zip-file to destroy"
		"^H^H^H^H^H^H^Htest");
	return 1;
    }

    seterrinfo(NULL, argv[1]);
    if ((zf=zip_open(argv[1], 0))==NULL) {
	myerror(ERRZIPSTR, "can't open file");
	return 1;
    }

    for (i=0; i<zf->nentry; i++)
	printf("%8d %s\n", zf->entry[i].uncomp_size, zf->entry[i].fn);

    zff = zff_open(zf, 1);
    i = zff_read(zff, buf, BUFSIZE-1);
    zff_close(zff);

    if (i < 0)
	fprintf(stderr, "read error: %s\n", zip_err_str[zff->flags]);
    else {
	buf[i] = 0;
	printf("read %d bytes: '%s'\n", i, buf);
    }
    
    if (zip_close(zf)!=0) {
	myerror(ERRZIPSTR, "can't close file");
	return 1;
    }
    
    return 0;
}
