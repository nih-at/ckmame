#include <stdlib.h>

#include "zip.h"
#include "xmalloc.h"

static int zip_unchange_data(struct zf *zf, int idx);



int
zip_rename(struct zf *zf, int idx, char *name)
{
    if (idx >= zf->nentry || idx < 0)
	return -1;

    if (zf->entry[idx].state == Z_UNCHANGED) 
	zf->entry[idx].state = Z_RENAMED;
    zf->changes = 1;

    if (zf->entry[idx].ch_name)
	free(zf->entry[idx].ch_name);
    
    zf->entry[idx].ch_name = xstrdup(name);

    return idx;
}



int
zip_add_file(struct zf *zf, char *name, FILE *file,
	int start, int len)
{
    if (zf->nentry >= zf->nentry_alloc-1) {
	zf->nentry_alloc += 16;
	zf->entry = (struct zf_entry *)xrealloc(zf->entry,
						 sizeof(struct zf_entry)
						 * zf->nentry_alloc);
    }

    zf->changes = 1;
    zf->entry[zf->nentry].state = Z_ADDED;
    zf->entry[zf->nentry].ch_name = xstrdup(name);
    zf->entry[zf->nentry].ch_data_fp = file;
    zf->entry[zf->nentry].ch_data_buf = NULL;
    zf->entry[zf->nentry].ch_data_offset = start;
    zf->entry[zf->nentry].ch_data_len = len;
    zf->nentry++;

    return zf->nentry;
}



int
zip_add_data(struct zf *zf, char *name, char *buf,
	int start, int len)
{
    if (zf->nentry >= zf->nentry_alloc-1) {
	zf->nentry_alloc += 16;
	zf->entry = (struct zf_entry *)xrealloc(zf->entry,
						 sizeof(struct zf_entry)
						 * zf->nentry_alloc);
    }

    zf->changes = 1;
    zf->entry[zf->nentry].state = Z_ADDED;
    zf->entry[zf->nentry].ch_name = xstrdup(name);
    zf->entry[zf->nentry].ch_data_fp = NULL;
    zf->entry[zf->nentry].ch_data_buf = buf;
    zf->entry[zf->nentry].ch_data_offset = start;
    zf->entry[zf->nentry].ch_data_len = len;
    zf->nentry++;

    return zf->nentry;
}



int
zip_add_zip(struct zf *zf, char *name, struct zf *zf1, int idx1,
	    int start, int len)
{
    if (idx1 >= zf1->nentry || idx1 < 0)
	return -1;

    if (zf->nentry >= zf->nentry_alloc-1) {
	zf->nentry_alloc += 16;
	zf->entry = (struct zf_entry *)xrealloc(zf->entry,
						 sizeof(struct zf_entry)
						 * zf->nentry_alloc);
    }

    zf->changes = 1;
    zf->entry[zf->nentry].state = Z_ADDED;
    zf->entry[zf->nentry].ch_name = xstrdup(name);
    zf->entry[zf->nentry].ch_data_fp = NULL;
    zf->entry[zf->nentry].ch_data_buf = NULL;
    zf->entry[zf->nentry].ch_data_zf = zf1;
    zf->entry[zf->nentry].ch_data_zf_fileno = idx1;
    zf->entry[zf->nentry].ch_data_offset = start;
    zf->entry[zf->nentry].ch_data_len = len;
    zf->nentry++;

    return zf->nentry;
}



int
zip_replace_file(struct zf *zf, int idx, char *name, FILE *file,
	    int start, int len)
{
    if (idx >= zf->nentry || idx < 0)
	return -1;

    zip_unchange_data(zf, idx);

    zf->changes = 1;
    zf->entry[idx].state = Z_REPLACED;
    if (name) {
	if (zf->entry[idx].ch_name)
	    free(zf->entry[idx].ch_name);
	zf->entry[idx].ch_name = xstrdup(name);
    }
    
    zf->entry[idx].ch_data_fp = file;
    zf->entry[idx].ch_data_offset = start;
    zf->entry[idx].ch_data_len = len;

    return idx;
}



int
zip_replace_data(struct zf *zf, int idx, char *name, char *buf,
	    int start, int len)
{
    if (idx >= zf->nentry || idx < 0)
	return -1;

    zip_unchange_data(zf, idx);

    zf->changes = 1;
    zf->entry[idx].state = Z_REPLACED;
    if (name) {
	if (zf->entry[idx].ch_name)
	    free(zf->entry[idx].ch_name);
	zf->entry[idx].ch_name = xstrdup(name);
    }
    zf->entry[idx].ch_data_buf = buf;
    zf->entry[idx].ch_data_offset = start;
    zf->entry[idx].ch_data_len = len;

    return idx;
}



int
zip_replace_zip(struct zf *zf, int idx, char *name, struct zf *zf1, int idx1,
	    int start, int len)
{
    if (idx >= zf->nentry || idx < 0)
	return -1;

    if (idx1 >= zf1->nentry || idx1 < 0)
	return -1;

    zip_unchange_data(zf, idx);

    zf->changes = 1;
    zf->entry[idx].state = Z_REPLACED;
    if (name) {
	if (zf->entry[idx].ch_name)
	    free(zf->entry[idx].ch_name);
	zf->entry[idx].ch_name = xstrdup(name);
    }
    zf->entry[idx].ch_data_zf = zf1;
    zf->entry[idx].ch_data_zf_fileno = idx1;
    zf->entry[idx].ch_data_offset = start;
    zf->entry[idx].ch_data_len = len;

    return idx;
}



int
zip_delete(struct zf *zf, int idx)
{
    if (idx >= zf->nentry || idx < 0)
	return -1;

    zip_unchange(zf, idx);

    zf->changes = 1;
    zf->entry[idx].state = Z_DELETED;

    return idx;
}



int
zip_unchange(struct zf *zf, int idx)
{
    if (idx >= zf->nentry || idx < 0)
	return -1;

    if (zf->entry[idx].ch_name) {
	free(zf->entry[idx].ch_name);
	zf->entry[idx].ch_name = NULL;
    }

    return zip_unchange_data(zf, idx);
}



static int
zip_unchange_data(struct zf *zf, int idx)
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

    zf->entry[idx].state = zf->entry[idx].ch_name ? Z_RENAMED : Z_UNCHANGED;

    return idx;
}
