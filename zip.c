#include "zip.h"



int
zip_rename(struct zipfile *zf, int idx, char *name)
{
    if (idx >= zf->nentry)
	return -1;

    if (zf->entry[idx].state == Z_UNCHANGED) 
	zf->entry[idx].state = Z_RENAMED;

    if (zf->entry[idx].ch_name)
	free(zf->entry[idx].ch_name);
    
    zf->entry[idx].ch_name = strdup(name);

    return idx;
}



int
zip_add_file(struct zipfile *zf, char *name, FILE *file,
	int start, int len)
{
    if (zf->nentry >= zf->entry_size-1) {
	zf->entry_size += 16;
	zf->entry = (struct zipchange *)xrealloc(zf->entry,
						 sizeof(struct zipchange)
						 * zf->entry_size);
    }

    zf->entry[zf->nentry].state = Z_ADDED;
    zf->entry[zf->nentry].ch_name = strdup(name);
    zf->entry[zf->nentry].ch_data_fp = file;
    zf->entry[zf->nentry].ch_data_buf = NULL;
    zf->entry[zf->nentry].ch_data_offset = start
    zf->entry[zf->nentry].ch_data_len = len;
    zf->nentry++;

    return zf->nentry;
}



int
zip_add_buf(struct zipfile *zf, char *name, char *buf,
	int start, int len)
{
    if (zf->nentry >= zf->entry_size-1) {
	zf->entry_size += 16;
	zf->entry = (struct zipchange *)xrealloc(zf->entry,
						 sizeof(struct zipchange)
						 * zf->entry_size);
    }

    zf->entry[zf->nentry].state = Z_ADDED;
    zf->entry[zf->nentry].ch_name = strdup(name);
    zf->entry[zf->nentry].ch_data_fp = NULL;
    zf->entry[zf->nentry].ch_data_buf = buf;
    zf->entry[zf->nentry].ch_data_offset = start
    zf->entry[zf->nentry].ch_data_len = len;
    zf->nentry++;

    return zf->nentry;
}



int
zip_replace_file(struct zipfile *zf, int idx, char *name, FILE *file,
	    int start, int len)
{
    if (idx >= zf->nentry)
	return -1;

    zf->entry[idx].state = Z_REPLACED;
    if (zf->entry[idx].ch_name)
	free(zf->entry[idx].ch_name);
    if (name)
	zf->entry[idx].ch_name = strdup(name);
    else
	zf->entry[idx].ch_name = NULL;
    zf->entry[idx].ch_data_fp = file;
    zf->entry[idx].ch_data_buf = NULL;
    zf->entry[idx].ch_data_offset = start;
    zf->entry[idx].ch_data_len = len;

    return idx;
}



int
zip_replace_data(struct zipfile *zf, int idx, char *name, char *buf,
	    int start, int len)
{
    if (idx >= zf->nentry)
	return -1;

    zf->entry[idx].state = Z_REPLACED;
    if (zf->entry[idx].ch_name)
	free(zf->entry[idx].ch_name);
    if (name)
	zf->entry[idx].ch_name = strdup(name);
    else
	zf->entry[idx].ch_name = NULL;
    zf->entry[idx].ch_data_fp = NULL;
    zf->entry[idx].ch_data_buf = buf;
    zf->entry[idx].ch_data_offset = start;
    zf->entry[idx].ch_data_len = len;

    return idx;
}



int
zip_delete(struct zipfile *zf, int idx)
{
    if (idx >= zf->nentry)
	return -1;

    if (zf->entry[idx].ch_name) {
	free(zf->entry[idx].ch_name);
	zf->entry[idx].ch_name = NULL;
    }

    zf->entry[idx].state = Z_DELETED;

    return idx;
}



int
zip_unchange(struct zipfile *zf, int idx)
{
    if (idx >= zf->nentry)
	return -1;

    if (zf->entry[idx].ch_name) {
	free(zf->entry[idx].ch_name);
	zf->entry[idx].ch_name = NULL;
    }

    zf->entry[idx].state = Z_UNCHANGED;

    return idx;
}
