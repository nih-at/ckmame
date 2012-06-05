/*
  chd.c -- accessing chd files
  Copyright (C) 2004-2012 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "chd.h"
#include "compat.h"



#define MAX_HEADERLEN	124		/* maximum header length */
#define TAG		"MComprHD"
#define TAG_LEN		8		/* length of tag */
#define TAG_AND_LEN	12		/* length of tag + header length */

#define META_HEADERLEN	16

#define MAP_ENTRY_SIZE_V12	8	/* size of map entry, versions 1 & 2 */
#define MAP_ENTRY_SIZE_V3	16	/* size of map entry, version 3 */

#define GET_UINT16(b)	(b+=2,((b)[-2]<<8)|(b)[-1])
#define GET_UINT32(b)	(b+=4,((b)[-4]<<24)|((b)[-3]<<16)|((b)[-2]<<8)|(b)[-1])
#define GET_UINT64(b)	(b+=8,((uint64_t)(b)[-8]<<56)|((uint64_t)(b)[-7]<<48) \
			 |((uint64_t)(b)[-6]<<40)|((uint64_t)(b)[-5]<<32)     \
			 |((uint64_t)(b)[-4]<<24)|((uint64_t)(b)[-3]<<16)     \
			 |((uint64_t)(b)[-2]<<8)|((uint64_t)(b)[-1]))

#define MAKE_TAG(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))
static uint32_t v4_compressors[] = {
    0,
    CHD_CODEC_ZLIB,
    CHD_CODEC_ZLIB, /* XXX: zlib plus */
    CHD_CODEC_AVHUFF
};

static uint8_t  v4_map_types[] = {
    0, CHD_MAP_TYPE_COMPRESSOR0, CHD_MAP_TYPE_UNCOMPRESSED, CHD_MAP_TYPE_MINI, CHD_MAP_TYPE_SELF_REF, CHD_MAP_TYPE_PARENT_REF
};

static uint8_t  v5_map_types[] = {
    CHD_MAP_TYPE_UNCOMPRESSED, CHD_MAP_TYPE_SELF_REF, CHD_MAP_TYPE_PARENT_REF, CHD_MAP_TYPE_MINI,
    CHD_MAP_TYPE_COMPRESSOR0, CHD_MAP_TYPE_COMPRESSOR1, CHD_MAP_TYPE_COMPRESSOR2, CHD_MAP_TYPE_COMPRESSOR3
};

static int read_header(struct chd *);
static int read_header_v5(struct chd *, unsigned char *, uint32_t);
static int read_map(struct chd *);
static int read_map_v5(struct chd *);



void
chd_close(struct chd *chd)
{
    fclose(chd->f);
    free(chd->name);
    free(chd->map);
    free(chd->buf);
    free(chd->hbuf);
    free(chd);
}   



struct chd_metadata_entry *
chd_get_metadata_list(struct chd *chd)
{
    /* XXX: handle/propagate error */
    read_meta_headers(chd);

    return chd->meta;
}



struct chd *
chd_open(const char *name, int *errp)
{
    struct chd *chd;
    FILE *f;

    if ((f=fopen(name, "rb")) == NULL) {
	if (errp)
	    *errp = CHD_ERR_OPEN;
	return NULL;
    }

    if ((chd=malloc(sizeof(*chd))) == NULL) {
	if (errp)
	    *errp = CHD_ERR_NOMEM;
	return NULL;
    }
    chd->f = f;
    if ((chd->name=strdup(name)) == NULL) {
	if (errp)
	    *errp = CHD_ERR_NOMEM;
	chd_close(chd);
	return NULL;
    }
    chd->error = 0;
    chd->map = NULL;
    chd->buf = NULL;
    chd->hno = -1;
    chd->hbuf = NULL;
    chd->meta = NULL;

    if (read_header(chd) < 0) {
	if (errp)
	    *errp = chd->error;
	chd_close(chd);
	return NULL;
    }

    return chd;
}



int
chd_read_hunk(struct chd *chd, int idx, unsigned char *b)
{
    int i, n, err;
    uint32_t compression_type;

    if (idx < 0 || (unsigned int)idx > chd->total_hunks) {
	chd->error = CHD_ERR_INVAL;
	return -1;
    }

    if (chd->map == NULL) {
	if (read_map(chd) < 0)
	    return -1;
    }

    if (chd->map[idx].length > chd->hunk_len) {
	chd->error = CHD_ERR_NOTSUP;
	return -1;
    }

    if (chd->map[idx].type < 4)
	compression_type = chd->compressors[chd->map[idx].type];
    else
	compression_type = chd->map[idx].type;
	
    switch (compression_type) {
    case CHD_CODEC_ZLIB:
	if (chd->buf == NULL) {
	    if ((chd->buf=malloc(chd->hunk_len)) == NULL) {
		chd->error = CHD_ERR_NOMEM;
		return -1;
	    }
	    chd->z.avail_in = 0;
	    chd->z.zalloc = Z_NULL;
	    chd->z.zfree = Z_NULL;
	    chd->z.opaque = NULL;
	    err = inflateInit2(&chd->z, -MAX_WBITS);
	}
	else
	    err = inflateReset(&chd->z);
	if (err != Z_OK) {
	    chd->error = CHD_ERR_ZLIB;
	    return -1;
	}

	if (fseeko(chd->f, chd->map[idx].offset, SEEK_SET) == -1) {
	    chd->error = CHD_ERR_SEEK;
	    return -1;
	}
	if ((n=fread(chd->buf, 1, chd->map[idx].length, chd->f)) < 0) {
	    chd->error = CHD_ERR_READ;
	    return -1;
	}

	chd->z.next_in = (Bytef *)chd->buf;
	chd->z.avail_in = n;
	chd->z.next_out = (Bytef *)b;
	chd->z.avail_out = chd->hunk_len;
	/* XXX: should use Z_FINISH, but that returns Z_BUF_ERROR */
	if ((err=inflate(&chd->z, 0)) != Z_OK && err != Z_STREAM_END) {
	    chd->error = CHD_ERR_ZLIB;
	    return -1;
	}
	/* XXX: chd->z.avail_out should be 0 */
	n = chd->hunk_len - chd->z.avail_out;
	break;

    case CHD_MAP_TYPE_UNCOMPRESSED:
	if (fseeko(chd->f, chd->map[idx].offset, SEEK_SET) == -1) {
	    chd->error = CHD_ERR_SEEK;
	    return -1;
	}
	/* XXX: use chd->hunk_len instead? */
	if ((n=fread(b, 1, chd->map[idx].length, chd->f)) < 0) {
	    chd->error = CHD_ERR_READ;
	    return -1;
	}
	break;

    case CHD_MAP_TYPE_MINI:
	b[0] = (chd->map[idx].offset >> 56) & 0xff;
	b[1] = (chd->map[idx].offset >> 48) & 0xff;
	b[2] = (chd->map[idx].offset >> 40) & 0xff;
	b[3] = (chd->map[idx].offset >> 32) & 0xff;
	b[4] = (chd->map[idx].offset >> 24) & 0xff;
	b[5] = (chd->map[idx].offset >> 16) & 0xff;
	b[6] = (chd->map[idx].offset >> 8) & 0xff;
	b[7] = chd->map[idx].offset & 0xff;
	n = chd->hunk_len;
	for (i=8; i<n; i++)
	    b[i] = b[i-8];
	break;

    case CHD_MAP_TYPE_SELF_REF:
	/* XXX: check CRC here too? */
	return chd_read_hunk(chd, chd->map[idx].offset, b);

    case CHD_MAP_TYPE_PARENT_REF:
	chd->error = CHD_ERR_NOTSUP;
	return -1;

    default:
	chd->error = CHD_ERR_NOTSUP;
	return -1;
    }
    
    if ((chd->map[idx].flags & CHD_MAP_FL_NOCRC) == 0) {
	if (crc32(0, (Bytef *)b, n) != chd->map[idx].crc) {
	    chd->error = CHD_ERR_CRC;
	    return -1;
	}
    }
    
    return n;
}



int
chd_read_metadata(struct chd* chd, const struct chd_metadata_entry *e,
		  unsigned char *b)
{
    if (e == NULL) {
	chd->error = CHD_ERR_INVAL;
	return -1;
    }

    if (chd->meta == NULL) {
	if (read_meta_headers(chd) < 0)
	    return -1;
    }

    if (fseeko(chd->f, e->offset, SEEK_SET) == -1) {
	chd->error = CHD_ERR_SEEK;
	return -1;
    }
    if (fread(b, e->length, 1, chd->f) != 1) {
	chd->error = CHD_ERR_READ;
	return -1;
    }

    return 0;
}



int
chd_read_range(struct chd *chd, unsigned char *b, int off, int len)
{
    int i, s, n;
    unsigned int copied, o2, l2;

    /* XXX: error handling */

    s = off/chd->hunk_len;
    n = (off+len+chd->hunk_len-1)/chd->hunk_len - s;

    copied = 0;
    o2 = off % chd->hunk_len;
    l2 = chd->hunk_len - o2;

    for (i=0; i<n; i++) {
	if (i == 1) {
	    o2 = 0;
	    l2 = chd->hunk_len;
	}
	if (i == n-1) {
	    if (l2 > len-copied)
		l2 = len-copied;
	}
	if (o2 == 0 && l2 == chd->hunk_len && s+i != chd->hno) {
	    if (chd_read_hunk(chd, s+i, b+copied) < 0)
		return -1;
	    copied += chd->hunk_len;
	}
	else {
	    if (chd->hbuf == NULL)
		if ((chd->hbuf=malloc(chd->hunk_len)) == NULL) {
		    chd->error = CHD_ERR_NOMEM;
		    return -1;
		}
	    if (s+i != chd->hno) {
		if (chd_read_hunk(chd, s+i, chd->hbuf) < 0)
		    return  -1;
		chd->hno = s+i;
	    }
	    memcpy(b+copied, chd->hbuf+o2, l2);
	    copied += l2;
	}
    }

    return len;
}



static int
read_header(struct chd *chd)
{
    uint32_t len;

    unsigned char b[MAX_HEADERLEN], *p;

    if (fread(b, TAG_AND_LEN, 1, chd->f) != 1) {
	chd->error = CHD_ERR_READ;
	return -1;
    }

    if (memcmp(b, TAG, TAG_LEN) != 0) {
	chd->error = CHD_ERR_NO_CHD;
	return -1;
    }

    p = b+TAG_LEN;
    len = GET_UINT32(p);
    if (len > MAX_HEADERLEN) {
	chd->error = CHD_ERR_NO_CHD;
	return -1;
    }
    if (fread(p, len-TAG_AND_LEN, 1, chd->f) != 1) {
	chd->error = CHD_ERR_READ;
	return -1;
    }
    
    chd->hdr_length = len;
    chd->version = GET_UINT32(p);

    if (chd->version > 5) {
	chd->error = CHD_ERR_VERSION;
	return -1;
    }
    
    if (chd->version >= 5)
	return read_header_v5(chd, b, len);
    
    chd->flags = GET_UINT32(p);
    chd->compressors[0] = v4_compressors[GET_UINT32(p)];

    /* XXX: check chd->hdr_length against expected value for version */

    if (chd->version < 3) {
	chd->hunk_len = GET_UINT32(p);
	chd->total_hunks = GET_UINT32(p);
	p += 12; /* skip c/h/s */
	memcpy(chd->md5, p, sizeof(chd->md5));
	p += sizeof(chd->md5);
	memcpy(chd->parent_md5, p, sizeof(chd->parent_md5));
	p += sizeof(chd->parent_md5);

	if (chd->version == 1)
	    chd->hunk_len *= 512;
	else
	    chd->hunk_len *= GET_UINT32(p);
	chd->total_len = chd->hunk_len * chd->total_hunks;
	chd->meta_offset = 0;
	memset(chd->sha1, 0, sizeof(chd->sha1));
	memset(chd->parent_sha1, 0, sizeof(chd->parent_sha1));
	memset(chd->raw_sha1, 0, sizeof(chd->raw_sha1));
    }
    else {
	chd->total_hunks = GET_UINT32(p);
	chd->total_len = GET_UINT64(p);
	chd->meta_offset = GET_UINT64(p);

	if (chd->version == 3) {
	    memcpy(chd->md5, p, sizeof(chd->md5));
	    p += sizeof(chd->md5);
	    memcpy(chd->parent_md5, p, sizeof(chd->parent_md5));
	    p += sizeof(chd->parent_md5);
	}
	else {
	    memset(chd->md5, 0, sizeof(chd->md5));
	    memset(chd->parent_md5, 0, sizeof(chd->parent_md5));
	}
	
	chd->hunk_len = GET_UINT32(p);
	
	memcpy(chd->sha1, p, sizeof(chd->sha1));
	p += sizeof(chd->sha1);
	memcpy(chd->parent_sha1, p, sizeof(chd->parent_sha1));
	p += sizeof(chd->parent_sha1);

	if (chd->version == 3)
	    memcpy(chd->raw_sha1, chd->sha1, sizeof(chd->raw_sha1));
	else {
	    memcpy(chd->raw_sha1, p, sizeof(chd->raw_sha1));
	    p += sizeof(chd->raw_sha1);
	}
    }

    chd->map_offset = chd->hdr_length;
    
    return 0;
}




static int
read_header_v5(struct chd *chd, unsigned char *header, uint32_t len)
{
    /* 
    V5 header:

    [  0] char   tag[8];        // 'MComprHD'
    [  8] UINT32 length;        // length of header (including tag and
				// length fields)
    [ 12] UINT32 version;       // drive format version
    [ 16] UINT32 compressors[4];// which custom compressors are used?
    [ 32] UINT64 logicalbytes;  // logical size of the data (in bytes)
    [ 40] UINT64 mapoffset;     // offset to the map
    [ 48] UINT64 metaoffset;    // offset to the first blob of
				// metadata
    [ 56] UINT32 hunkbytes;     // number of bytes per hunk (512k
				// maximum)
    [ 60] UINT32 unitbytes;     // number of bytes per unit within
				// each hunk
    [ 64] UINT8  rawsha1[20];   // raw data SHA1
    [ 84] UINT8  sha1[20];      // combined raw+meta SHA1
    [104] UINT8  parentsha1[20];// combined raw+meta SHA1 of parent
    [124] (V5 header length)
    */

    unsigned char *p = header + TAG_AND_LEN + 4;
    int i;
    
    for (i=0; i<4; i++)
	chd->compressors[i] = GET_UINT32(p);

    chd->total_len = GET_UINT64(p);

    chd->map_offset = GET_UINT64(p);
    chd->meta_offset = GET_UINT64(p);

    chd->hunk_len = GET_UINT32(p);
    chd->total_hunks = (chd->total_len+chd->hunk_len-1) / chd->hunk_len;
    
    p += 4; /* unit bytes */
    
    memcpy(chd->raw_sha1, p, sizeof(chd->raw_sha1));
    p += sizeof(chd->raw_sha1);
    memcpy(chd->sha1, p, sizeof(chd->sha1));
    p += sizeof(chd->sha1);
    memcpy(chd->parent_sha1, p, sizeof(chd->parent_sha1));
    p += sizeof(chd->parent_sha1);

    chd->flags = 0;
    for (i=0; i<sizeof(chd->parent_sha1); i++)
	if (chd->parent_sha1[i] != 0) {
	    chd->flags = CHD_FLAG_HAS_PARENT;
	    break;
	}
    
    return 0;
}



static int
read_map(struct chd *chd)
{
    unsigned char b[MAP_ENTRY_SIZE_V3], *p;
    unsigned int i, len;
    uint64_t v;

    if (fseek(chd->f, chd->map_offset, SEEK_SET) < 0) {
	chd->error = CHD_ERR_SEEK;
	return -1;
    }

    if ((chd->map=malloc(sizeof(*chd->map)*chd->total_hunks)) == NULL) {
	chd->error = CHD_ERR_NOMEM;
	return -1;
    }

    if (chd->version >= 5)
	return read_map_v5(chd);

    if (chd->version < 3)
	len = MAP_ENTRY_SIZE_V12;
    else
	len = MAP_ENTRY_SIZE_V3;

    for (i=0; i<chd->total_hunks; i++) {
	if (fread(b, len, 1, chd->f) != 1) {
	    chd->error = CHD_ERR_READ;
	    return -1;
	}
	p = b;

	/* XXX: why? */
	if (i == 1832 && chd->version < 3)
	    chd->version = 3;

	if (chd->version < 3) {
	    v = GET_UINT64(p);
	    chd->map[i].offset = v & 0xFFFFFFFFFFFLL;
	    chd->map[i].crc = 0;
	    chd->map[i].length = v >> 44;
	    chd->map[i].flags = CHD_MAP_FL_NOCRC;
	    if (chd->map[i].length == chd->hunk_len)
		chd->map[i].type = CHD_MAP_TYPE_UNCOMPRESSED;
	    else
		chd->map[i].type = CHD_MAP_TYPE_COMPRESSOR0;
	}
	else {
	    chd->map[i].offset = GET_UINT64(p);
	    chd->map[i].crc = GET_UINT32(p);
	    chd->map[i].length = GET_UINT16(p);
	    chd->map[i].flags = GET_UINT16(p);
	    chd->map[i].type = v4_map_types[chd->map[i].flags & 0x0f];
	    chd->map[i].flags &= 0xf0;
	}
    }

    return 0;
}



static int
read_map_v5(struct chd *chd)
{
    int i;

    chd->error = CHD_ERR_NOTSUP;
    
    if (chd->compressors[0] == 0) {
	/* XXX: uncompressed map */

	return -1;
    }
    return -1;
}



int
read_meta_headers(struct chd *chd)
{
    struct chd_metadata_entry *meta, *prev;
    uint64_t offset, next;
    unsigned char b[META_HEADERLEN], *p;

    if (chd->meta != NULL)
	return 0; /* already read in */

    prev = NULL;
    for (offset = chd->meta_offset; offset; offset = next) {
	if (fseeko(chd->f, offset, SEEK_SET) == -1) {
	    chd->error = CHD_ERR_SEEK;
	    return -1;
	}
	if (fread(b, META_HEADERLEN, 1, chd->f) != 1) {
	    chd->error = CHD_ERR_READ;
	    return -1;
	}

	if ((meta=malloc(sizeof(*meta))) == NULL) {
	    chd->error = CHD_ERR_NOMEM;
	    return -1;
	}

	p = b;

	meta->next = NULL;
	memcpy(meta->tag, p, 4);
	p += 4;
	meta->length = GET_UINT32(p);
	meta->offset = offset + META_HEADERLEN;
	meta->flags = meta->length >> 24;
	meta->length &= 0xffffff;

	next = GET_UINT64(p);

	if (prev)
	    prev->next = meta;
	else
	    chd->meta = meta;
	prev = meta;
    }

    return 0;
}
