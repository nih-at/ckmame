#ifndef _HAD_ZIP_H
#define _HAD_ZIP_H

#include <sys/types.h>
#include <stdio.h>

#include "unzip.h"

enum zip_state { Z_UNCHANGED, Z_DELETED, Z_REPLACED, Z_ADDED, Z_RENAMED };

int zip_err; /* global variable for errors returned by the low-level
		library */

/* 0 is no error */
#define ZERR_MULTIDISK        1
#define ZERR_RENAME           2
#define ZERR_CLOSE            3
#define ZERR_SEEK             4
#define ZERR_READ             5
#define ZERR_WRITE            6

extern char * zip_err_str[];

struct zipfile {
    char *name;
    unzFile zfp;
    int entry_size, nentry;
    struct zipchange *entry;
};

struct zipchange {
    enum zip_state state;
    char *name;
    struct zipfile *szf;
    int sindex; /* which file in zipfile */
    int start, len;
};

struct zf {
    char *zn;
    FILE *zp;
    unsigned short comlen, changes;
    unsigned int nentry, nentry_alloc, cd_size, cd_offset;
    unsigned char *com;
    struct zf_entry *entry;
};

struct zf_entry {
    unsigned short version_made, version_need, bitflags, comp_meth,
	lmtime, lmdate, fnlen, eflen, fcomlen, disknrstart, intatt;
    unsigned int crc, comp_size, uncomp_size, extatt, local_offset;
    enum zip_state state;
    char *fn, *ef, *fcom;
    char *ch_name;
    /* only use one of the following two for supplying new data */
    FILE *ch_data_fp;
    char *ch_data_buf;
    unsigned int ch_data_offset, ch_data_len;
};

#endif /* _HAD_ZIP_H */
