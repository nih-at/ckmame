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
    if ((zf=zip_open(argv[1], 0))==NULL) {
	myerror(ERRZIPSTR, "can't open file");
	return 1;
    }

    for (i=0; i<zf->nentry; i++)
	printf("%8d %s\n", zf->entry[i].uncomp_size, zf->entry[i].fn);

    zip_delete(zf, 0);
    
    if (zip_close(zf)!=0) {
	myerror(ERRZIPSTR, "can't close file");
	return 1;
    }
    
    return 0;
}
