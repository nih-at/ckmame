#ifndef _HAD_TYPES_H
#define _HAD_TYPES_H

enum where {
    ROM_INZIP, ROM_INCO, ROM_INGCO
};

enum state {
    ROM_0,
    ROM_UNKNOWN, ROM_SHORT, ROM_LONG, ROM_CRCERR,
    ROM_NAMERR, ROM_LONGOK, ROM_OK
};

struct rom {
    char *name;
    unsigned long size, crc;
    enum state state;
    enum where where;
};
    
struct game {
    char *name;
    char *cloneof[2];
    int nclone;
    char **clone;
    struct rom *rom;
    int nrom;
    char  *sampleof;
    struct rom *sample;
    int nsample;
};

struct match {
    struct match *next;
    enum state where;
    int zno;
    int fno;
    enum state quality;
};

struct zip {
    char *name;
    struct rom *rom;
    int nrom;
};

struct tree {
    char *name;
    struct tree *next, *child;
};

#endif /* types.h */
