#include "zip.h"

static int zip_data_unchange(struct zipfile *zf, int idx);



int
zip_rename(struct zipfile *zf, int idx, char *name)
{
    if (idx >= zf->nentry || idx < 0)
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
zip_add_data(struct zipfile *zf, char *name, char *buf,
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
    if (idx >= zf->nentry || idx < 0)
	return -1;

    zip_unchange_data(zf, idx)

    zf->entry[idx].state = Z_REPLACED;
    if (name) {
	if (zf->entry[idx].ch_name)
	    free(zf->entry[idx].ch_name);
	zf->entry[idx].ch_name = strdup(name);
    }
    
    zf->entry[idx].ch_data_fp = file;
    zf->entry[idx].ch_data_offset = start;
    zf->entry[idx].ch_data_len = len;

    return idx;
}



int
zip_replace_data(struct zipfile *zf, int idx, char *name, char *buf,
	    int start, int len)
{
    if (idx >= zf->nentry || idx < 0)
	return -1;

    zip_unchange_data(zf, idx)

    zf->entry[idx].state = Z_REPLACED;
    if (name) {
	if (zf->entry[idx].ch_name)
	    free(zf->entry[idx].ch_name);
	zf->entry[idx].ch_name = strdup(name);
    }
    zf->entry[idx].ch_data_buf = buf;
    zf->entry[idx].ch_data_offset = start;
    zf->entry[idx].ch_data_len = len;

    return idx;
}



int
zip_delete(struct zipfile *zf, int idx)
{
    if (idx >= zf->nentry || idx < 0)
	return -1;

    zip_unchange(zf, idx)

    zf->entry[idx].state = Z_DELETED;

    return idx;
}



int
zip_unchange(struct zipfile *zf, int idx)
{
    if (idx >= zf->nentry || idx < 0)
	return -1;

    if (zf->entry[idx].ch_name) {
	free(zf->entry[idx].ch_name);
	zf->entry[idx].ch_name = NULL;
    }

    return zip_data_unchange(zf, idx);
}


static int
zip_data_unchange(struct zipfile *zf, int idx)
{
    if (idx >= zf->nentry || idx < 0)
	return -1;

    if (zf->entry[idx].ch_data_fp) {
	fclose(zf->entry[idx].ch_data_fp);
	zf->entry[idx].ch_data_fp = NULL;
    }
    if (zf->entry[idx].ch_data_buf) {
	free(zf->entry[idx].ch_data_buf);
	zf->entry[idx].ch_data_buf = NULL;
    }

    zf->entry[idx].ch_data_zf = NULL;
    zf->entry[idx].ch_data_zf_fileno = 0;
    zf->entry[idx].ch_data_offset = 0;
    zf->entry[idx].ch_data_len = 0;

    zf->entry[idx].state = zf->entry[idx].name ? Z_RENAMED : Z_UNCHANGED;

    return idx;
}
