#include <stdlib.h>
#include <string.h>

#include "unzip.h"
#include "types.h"
#include "error.h"

#define MAXFNLEN 1024

void
freeroms(struct rom *romp, int count)
{
    int i;
    
    for (i=0; i<count; i++)
	free(romp[i].name);

    free(romp);

    return;
}

int
readinfosfromzip (struct rom **rompp, char *zipfile)
{
    unzFile zfp;
    unz_global_info globinfo;
    int count, a;
    struct rom *romp;
    unz_file_info fileinfo;
    char filename[MAXFNLEN];
    
    if ((zfp=unzOpen(zipfile))==NULL)
	return -1;

    seterrinfo(NULL, zipfile);
    
    if (unzGetGlobalInfo(zfp, &globinfo)!=UNZ_OK) {
	myerror(ERRZIP, "error getting global file info");
	return -1;
    }

    if (globinfo.number_entry > 0) {
	if ((romp=malloc(sizeof(struct rom)*globinfo.number_entry))==NULL) {
	    myerror(ERRDEF, "malloc for %d roms failed",
		    globinfo.number_entry);
	    return -1;
	}
    }
    else {
	myerror(ERRZIP, "%d roms in zipfile (?)", globinfo.number_entry);
	return -1;
    }

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
	if ((romp[count].name=malloc(strlen(filename)+1))==NULL) {
	    myerror(ERRDEF, "malloc failure, can't reserve %d bytes",
		      strlen(filename)+1);
	    freeroms(romp, count);
	    return -1;
	}
	strncpy(romp[count].name, filename, strlen(filename));
	romp[count].name[strlen(filename)+1] = 0;
	romp[count].size = fileinfo.uncompressed_size;
	romp[count].crc = fileinfo.crc;
	romp[count].state = ROM_0;
    } while (((a=unzGoToNextFile(zfp))==UNZ_OK)
	     && (count<globinfo.number_entry));

    if (a!=UNZ_END_OF_LIST_OF_FILE) {
	myerror(ERRZIP, "can't go to next file after entry #%d", count);
	freeroms(romp, count);
	return -1;
    }
    
    if (unzClose(zipfile)!=UNZ_OK) {
	myerror(ERRZIP, "error closing file");
	free(romp);
	return -1;
    }

    *rompp = romp;
    return 0;
}
