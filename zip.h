#ifndef _HAD_ZIP_H
#define _HAD_ZIP_H

#include <sys/types.h>
#include <stdio.h>
#include <zlib.h>

/* #include "unzip.h" */

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
#define ZERR_CRC              7
#define ZERR_ZIPCLOSED        8

extern char *zip_err_str[];

struct zf {
    char *zn;
    FILE *zp;
    unsigned short comlen, changes;
    unsigned int cd_size, cd_offset;
    char *com;
    int nentry, nentry_alloc;
    struct zf_entry *entry;
    int nfile, nfile_alloc;
    struct zf_file **file;
};

struct zf_file {
    struct zf *zf;
    char *name;
    int flags; /* -1: eof, >0: error */

    int method;
    /* position within zip file (fread/fwrite) */
    long fpos;
    /* no of bytes left to read */
    unsigned long bytes_left;
    /* no of bytes of compressed data left */
    unsigned long cbytes_left;
    /* crc so far */
    unsigned long crc, crc_orig;
    
    char *buffer;
    z_stream *zstr;
};

struct zf_entry {
    unsigned short version_made, version_need, bitflags, comp_meth,
	lmtime, lmdate, fnlen, eflen, fcomlen, disknrstart, intatt;
    unsigned int crc, comp_size, uncomp_size, extatt, local_offset;
    enum zip_state state;
    char *fn, *ef, *fcom;
    char *ch_name;
    /* only use one of the following three for supplying new data
       listed in order of priority, if more than one is set */
    struct zf *ch_data_zf;
    char *ch_data_buf;
    FILE *ch_data_fp;
    /* offset & len of new data in ch_data_fp or ch_data_buf */
    unsigned int ch_data_offset, ch_data_len;
    /* if source is another zipfile, number of file in zipfile */
    unsigned int ch_data_zf_fileno;
};

int zip_unchange(struct zf *zf, int idx);
int zip_rename(struct zf *zf, int idx, char *name);
int zip_add_file(struct zf *zf, char *name, FILE *file, int start, int len);
int zip_add_data(struct zf *zf, char *name, char *buf, int start, int len);
int zip_replace_file(struct zf *zf, int idx, char *name, FILE *file,
		     int start, int len);
int zip_replace_data(struct zf *zf, int idx, char *name, char *buf,
		     int start, int len);
int zip_delete(struct zf *zf, int idx);


#endif /* _HAD_ZIP_H */
