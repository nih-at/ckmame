struct zf_file *zff_open(struct zf *zf, int fileno);
int zff_read(struct zf_file *zff, char *outbuf, int toread);
int zff_close(struct zf_file *zff);
