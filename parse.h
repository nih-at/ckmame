#ifndef HAD_PARSE_H
#define HAD_PARSE_H

#include <stdio.h>

#include "dbl.h"

/* parser functions */

int parse(DB *, const char *);
int parse_xml(FILE *f);
int parse_cm(FILE *f);

/* callbacks */

int parse_disk_end(void);
int parse_disk_md5(const char *);
int parse_disk_name(const char *);
int parse_disk_sha1(const char *);
int parse_disk_start(void);
int parse_game_cloneof(const char *);
int parse_game_description(const char *);
int parse_game_end(void);
int parse_game_name(const char *);
int parse_game_sampleof(const char *);
int parse_game_start(void);
int parse_prog_name(const char *);
int parse_prog_version(const char *);
int parse_rom_crc(const char *);
int parse_rom_end(void);
int parse_rom_flags(const char *);
int parse_rom_md5(const char *);
int parse_rom_merge(const char *);
int parse_rom_name(const char *);
int parse_rom_sha1(const char *);
int parse_rom_size(const char *);
int parse_rom_start(void);
int parse_sample_end(void);
int parse_sample_name(const char *);
int parse_sample_start(void);

extern char *parse_errstr;

#endif /* parse.h */
