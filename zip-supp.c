#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "unzip.h"
#include "types.h"
#include "error.h"

#define MAXFNLEN 1024
#define BUFSIZE 8192

void
freeroms(struct rom *romp, int count)
{
    int i;
    
    for (i=0; i<count; i++)
	free(romp[i].name);

    free(romp);

    return;
}



long
makencrc (char *zn, char *fn, int n)
{
    long crc;
    unzFile zfp;
    char buf[BUFSIZE];
    int left;
    
    if ((zfp=unzOpen(zfp))==NULL)
	return -1;

    if (unzLocateFile(zfp, fn, 1)!=UNZ_OK) {
	unzClose(zfp);
	return -1;
    }

    if (unzOpenCurrentFile(zfp)!=UNZ_OK) {
	unzClose(zfp);
	return -1;
    }

    left = BUFSIZE;
    crc = crc32(0, NULL, 0);

    while (n > 0) {
	if (left > n)
	    left = n;
	if (unzReadCurrentFile(zfp, buf, left)<left) {
	    unzCloseCurrentFile(zfp);
	    unzClose(zfp);
	    return -1;
	}
	crc = crc32(crc, buf, left);
	n -= left;
    }
    
    if (unzCloseCurrentFile(zfp)!=UNZ_OK) {
	unzClose(zfp);
	return -1;
    }
    
    if (unzClose(zfp)!=UNZ_OK) {
	myerror(ERRZIP, "error closing file");
	return -1;
    }

    return crc;
}




int
readinfosfromzip (struct rom **rompp, char *zipfile)
{
    unzFile zfp;
    unz_global_info globinfo;
    unz_file_info fileinfo;
    int count, a;
    struct rom *romp;
    char filename[MAXFNLEN+1];
    
    if ((zfp=unzOpen(zipfile))==NULL)
	return -1;

    seterrinfo(NULL, zipfile);
    
    if (unzGetGlobalInfo(zfp, &globinfo)!=UNZ_OK) {
	myerror(ERRZIP, "error getting global file info");
	return -1;
    }

    if (globinfo.number_entry > 0)
	romp = xmalloc(sizeof(struct rom)*globinfo.number_entry);
    else {
	myerror(ERRZIP, "%d roms in zipfile (?)", globinfo.number_entry);
	return -1;
    }

    for (a=0; a<globinfo.number_entry; a++)
	romp[a].name = NULL;
    
    if (unzGoToFirstFile(zfp)!=UNZ_OK) {
	myerror(ERRZIP, "can't go to first entry");
	free(romp);
	return -1;
    }

    count = -1;
    do {
	count++;
	if (unzGetCurrentFileInfo(zfp, &fileinfo, filename, MAXFNLEN,
				  NULL, 0, NULL, 0)!=UNZ_OK) {
	    myerror(ERRZIP, "can't get file info for entry #%d", count);
	    freeroms(romp, count);
	    return -1;
	}
	if (fileinfo.size_filename > MAXFNLEN)
	    fileinfo.size_filename = MAXFNLEN;
	filename[fileinfo.size_filename] = 0;
	romp[count].name = strdup(filename);
	romp[count].size = fileinfo.uncompressed_size;
	romp[count].crc = fileinfo.crc;
	romp[count].state = ROM_0;
    } while (((a=unzGoToNextFile(zfp))==UNZ_OK)
	     && (count<globinfo.number_entry));
    count++;
    
    if (a!=UNZ_END_OF_LIST_OF_FILE) {
	myerror(ERRZIP, "can't go to next file after entry #%d", count);
	freeroms(romp, count);
	return -1;
    }
    
    if (unzClose(zfp)!=UNZ_OK) {
	myerror(ERRZIP, "error closing file");
	free(romp);
	return -1;
    }

    *rompp = romp;
    return count;
}
