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
    struct zf_entry *entry;
};

struct zf_entry {
    ushort version_made, version_need, bitflags, comp_meth,
	lmtime, lmdate, fnlen, eflen, fcomlen, disknrstart, intatt;
    uint crc, comp_size, uncomp_size, extatt, local_offset;
    char *fn, *ef, *fcom;
}
