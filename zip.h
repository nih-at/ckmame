#include <sys/types.h>
#include <stdio.h>

#include "unzip.h"

enum zip_state { Z_UNCHANGED, Z_DELETED, Z_REPLACED, Z_ADDED, Z_RENAMED };

int zip_err; /* global variable for errors returned by the low-level
		library */

/* 0 is no error */
#define ZERR_MULTIDISK        1

char * zip_err_str[]={
    "no error",
    "multi-disk zip-files not supported"
};

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
    u_short nentry, com_size, changes;
    u_int cd_size, cd_offset;
    char *com;
    struct zf_entry *entry;
};

struct zf_entry {
    ushort version_made, version_need, bitflags, comp_meth,
	lmtime, lmdate, fnlen, eflen, fcomlen, disknrstart, intatt;
    uint crc, comp_size, uncomp_size, extatt, local_offset;
    char *fn, *ef, *fcom;
    enum zip_state state;
};

