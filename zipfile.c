#include <stdlib.h>
#include "error.h"
#include "xmalloc.h"
#include "ziplow.h"
#include "zip.h"
#include "zipfile.h"

int
zf_close(struct zf *zf)
{
    if (zf->unz_zst)
	inflateEnd(zf->unz_zst);
    free(zf->unz_in);
    zf->unz_in = NULL;
    free(zf->unz_zst);
    zf->unz_zst = NULL;
    zf->unz_last = -1;

    return 0;
}



int
zf_open(struct zf *zf, int fileno)
{
    unsigned char buf[4], *c;
    int offset, ret, len;

    if ((fileno < 0) || (fileno >= zf->nentry))
	return 0;

    if (zf->unz_last != -1)
	zf_close(zf);

    zf->unz_read = 0;
    seterrinfo(zf->entry[fileno].fn, zf->zn);

    if ((zf->entry[fileno].comp_meth != 0)
	&& (zf->entry[fileno].comp_meth != 8)) {
	myerror(ERRFILE, "unsupported compression method %d",
		zf->entry[fileno].comp_meth);
	return -1;
    }

    /* go to start of actual data */
    fseek (zf->zp, zf->entry[fileno].local_offset+LENTRYSIZE-4, SEEK_SET);
    len = fread(buf, 1, 4, zf->zp);
    if (len != 4) {
	myerror (ERRSTR, "can't read local header");
	return -1;
    }
    c = buf;
    offset = read2(&c);
    offset += read2(&c);
    fseek(zf->zp, offset, SEEK_CUR);
	
    if (zf->entry[fileno].comp_meth == 0) {
	zf->unz_last = fileno;
	return 0;
    }
    
    zf->unz_in = (char *)xmalloc(BUFSIZE);

    len = (BUFSIZE < zf->entry[fileno].comp_size ? BUFSIZE
	   : zf->entry[fileno].comp_size);
    ret = fread (zf->unz_in, 1, len, zf->zp);
    if (ret != len) {
	myerror (ERRSTR, "read error");
	free(zf->unz_in);
	zf->unz_in = NULL;
	return -1;
    }

    zf->unz_zst = (z_stream *)xmalloc(sizeof(z_stream));
    zf->unz_zst->zalloc = Z_NULL;
    zf->unz_zst->zfree = Z_NULL;
    zf->unz_zst->opaque = NULL;
    zf->unz_zst->next_in = zf->unz_in;
    zf->unz_zst->avail_in = len;

    /* negative value to tell zlib that there is no header */
    if (inflateInit2(zf->unz_zst, -MAX_WBITS) != Z_OK) {
	myerror(ERRFILE, zf->unz_zst->msg);
	zf_close(zf);
	return -1;
    }
    
    zf->unz_last = fileno;
    return 0;
}




int
zf_read(struct zf *zf, char *outbuf, int toread)
{
    int len, out_before, ret;

    if (zf->unz_last == -1)
	return -1;

    if (zf->unz_read == -1)
	return -1;
    
    if (zf->entry[zf->unz_last].comp_meth == 0) {
	if (toread > zf->entry[zf->unz_last].uncomp_size
	    - zf->unz_read)
	    len = zf->entry[zf->unz_last].uncomp_size - zf->unz_read;
	else
	    len = toread;

	ret = fread(outbuf, 1, len, zf->zp);
	if (ret == -1)
	    zf->unz_read = -1;
	else
	    zf->unz_read += ret;

	return ret;
    }
    
    zf->unz_zst->next_out = outbuf;
    zf->unz_zst->avail_out = toread;
    out_before = zf->unz_zst->total_out;
    
    /* endless loop until something has been accomplished */
    for (;;) {
	ret = inflate(zf->unz_zst, Z_SYNC_FLUSH);

	switch (ret) {
	case Z_OK:
	case Z_STREAM_END:
	    /* all ok */
	    /* XXX: STREAM_END probably won't happen, since we didn't
	       have a header */
	    return(zf->unz_zst->total_out - out_before);
	case Z_BUF_ERROR:
	    if (zf->unz_zst->avail_in == 0) {
		/* read some more bytes */
		len = fread (zf->unz_in, 1, BUFSIZE, zf->zp);
		if (len <= 0) {
		    myerror (ERRSTR, "read error");
		    return -1;
		}
		zf->unz_zst->next_in = zf->unz_in;
		zf->unz_zst->avail_in = len;
		continue;
	    }
	    myerror(ERRFILE, "zlib error: buf_err: %s", zf->unz_zst->msg);
	    return -1;
	case Z_NEED_DICT:
	case Z_DATA_ERROR:
	case Z_STREAM_ERROR:
	case Z_MEM_ERROR:
	    myerror(ERRFILE, "zlib error: %s", zf->unz_zst->msg);
	    return -1;
	}
    }
}

