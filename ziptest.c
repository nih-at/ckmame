#include "zip.h"
#include "error.h"
#include "ziplow.h"

char *prg;

int
main(int argc, char *argv[])
{
    int i;
    struct zf *zf;

    prg = argv[0];
    
    if (argc != 2) {
	myerror(ERRDEF, "call with one option, the zip-file to test");
	return 1;
    }

    seterrinfo(NULL, argv[1]);
    if ((zf=zip_open(argv[1]))==NULL) {
	myerror(ERRZIPSTR, "can't open file");
	return 1;
    }

    for (i=0; i<zf->nentry; i++)
	printf("%s\n", zf->entry[i].fn);

    if (zip_close(zf)!=0) {
	myerror(ERRZIPSTR, "can't close file");
	return 1;
    }
    
    return 0;
}
