enum state {
    ROM_0, ROM_INZIP, ROM_INCO, ROM_INRO,
    ROM_OK, ROM_CRCERR, ROM_LONG, ROM_SHORT, ROM_NAMERR, ROM_UNKNOWN
};

struct rom {
    char *name;
    long size, crc;
    enum state state;
};
    
struct game {
    char *name;
    char *cloneof, *romof;
    struct rom *rom;
    char  *sampleof;
    struct rom *sample;
};
