#ifndef _HAD_TYPES_H
#define _HAD_TYPES_H

#define WARN_CORRECT		0x1000

#define WARN_UNKNOWN		0x0001
#define WARN_USED		0x0002
#define WARN_NOT_USED		0x0004

#define WARN_SUPERFLUOUS	(WARN_UNKNOWN|WARN_USED|WARN_NOT_USED)

#define WARN_WRONG_ZIP		0x0008
#define WARN_WRONG_NAME		0x0010
#define WARN_LONGOK		0x0080

#define WARN_FIXABLE		(WARN_WRONG_ZIP|WARN_WRONG_NAME|WARN_LONGOK)

#define WARN_WRONG_CRC		0x0020
#define WARN_LONG		0x0040
#define WARN_SHORT		0x0100
#define WARN_MISSING		0x0200
#define WARN_NO_GOOD_DUMP       0x0400

#define WARN_BROKEN		(WARN_WRONG_CRC|WARN_LONG|WARN_SHORT\
				 |WARN_MISSING|WARN_NO_GOOD_DUMP)

#define WARN_ALL		(WARN_SUPERFLUOUS|WARN_FIXABLE|WARN_BROKEN)



enum where {
    ROM_INZIP, ROM_INCO, ROM_INGCO
};

enum state {
    ROM_0,
    ROM_UNKNOWN, ROM_SHORT, ROM_LONG, ROM_CRCERR,
    ROM_NAMERR, ROM_LONGOK, ROM_OK, ROM_TAKEN
};

struct rom {
    char *name, *merge;
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
    enum where where;
    int zno;
    int fno;
    enum state quality;
    int offset;              /* offset of correct part if ROM_LONGOK */
};

struct zip {
    char *name;
    struct rom *rom;
    int nrom;
};

struct tree {
    char *name;
    int check;
    struct tree *next, *child;
};



extern int output_options;

#endif /* types.h */
