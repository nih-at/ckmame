enum zipstate = { Z_UNCHANGED, Z_DELETED, Z_REPLACED, Z_ADDED, Z_RENAMED };

struct zipfile {
    char *name;
    unzFile zfp;
    int entry_size, nentry;
    struct zipchange *entry;
};

struct zipchange {
    enum zipstate state;
    char *name;
    struct zipfile *szf;
    int sindex; /* which file in zipfile */
    int start, len;
};

struct zf {
    u_short nentry, com_size;
    u_int cd_size, cd_offset;
    char *com;
}
