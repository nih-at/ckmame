#include "zip.h"



struct zipfile *
zip_open(char *zipname)
{
    struct zipfile *zf;
    unz_global_info globinfo;

    zf = (struct zipfile *)xmalloc(sizeof(struct zipfile));

    zf->zfp = unzOpen(zn);
    if (zf->zfp == NULL) {
	free(zf);
	return NULL;
    }

    zf->zipname = strdup(zipname);

    seterrinfo(NULL, zipname);
    
    if (unzGetGlobalInfo(zf->zfp, &globinfo)!=UNZ_OK) {
	myerror(ERRZIP, "error getting global file info");
	return NULL;
    }

    if (globinfo.number_entry <= 0) {
	myerror(ERRZIP, "%d roms in zipfile (?)", globinfo.number_entry);
	return NULL;
    }

    zf->nentry = globinfo.number_entry;
    zf->entry_size = (zf->nentry/16 + 1) * 16;
    zf->romp = (struct zipchange *)xmalloc(sizeof(struct zipchange)
					   * zf->entry_size);

    for (i=0; i<zf->nentry; i++) {
	zf->entry[i].state = Z_UNCHANGED;
	zf->entry[i].name = NULL;
	zf->entry[i].z = NULL;
	zf->entry[i].fzindex = zf->entry[i].start = 0;
	zf->entry[i].len = -1;
    }

    return zf;
}



int
zip_rename(struct zipfile *zf, int idx, char *name)
{
    if (idx >= zf->nentry)
	return -1;

    if (zf->entry[idx].state == Z_UNCHANGED) 
	zf->entry[idx].state = Z_RENAMED;

    if (zf->entry[idx].name)
	free(zf->entry[idx].name);
    
    zf->entry[idx].name = strdup(name);

    return idx;
}



int
zip_add(struct zipfile *zf, char *name, struct zipfile *szf,
	int sidx, int start, int len)
{
    if (zf->nentry >= zf->entry_size-1) {
	zf->entry_size += 16;
	zf->entry = (struct zipchange *)xrealloc(zf->entry,
						 sizeof(struct zipchange)
						 * zf->entry_size);
    }

    zf->entry[zf->nentry].state = Z_ADDED;
    zf->entry[zf->nentry].name = strdup(name);
    zf->entry[zf->nentry].szf = szf;
    zf->entry[zf->nentry].sindex = sidx;
    zf->entry[zf->nentry].start = start
    zf->entry[zf->nentry].len = len;
    zf->nentry++;

    return zf->nentry;
}



int
zip_replace(struct zipfile *zf, int idx, char *name, struct zipfile *szf,
	    int sidx, int start, int len)
{
    if (idx >= zf->nentry)
	return -1;

    zf->entry[idx].state = Z_REPLACED;
    if (zf->entry[idx].name)
	free(zf->entry[idx].name);
    zf->entry[idx].name = strdup(name);
    zf->entry[idx].szf = szf;
    zf->entry[idx].sindex = sidx;
    zf->entry[idx].start = start;
    zf->entry[idx].len = len;

    return idx;
}



int
zip_delete(struct zipfile *zf, int idx)
{
    if (idx >= zf->nentry)
	return -1;

    if (zf->entry[idx].name) {
	free(zf->entry[idx].name);
	zf->entry[idx].name = NULL;
    }

    zf->entry[idx].state = Z_DELETED;

    return idx;
}



int
zip_unchange(struct zipfile *zf, int idx)
{
    if (idx >= zf->nentry)
	return -1;

    if (zf->entry[idx].name) {
	free(zf->entry[idx].name);
	zf->entry[idx].name = NULL;
    }

    zf->entry[idx].state = Z_UNCHANGED;

    return idx;
}
