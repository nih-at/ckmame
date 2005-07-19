#ifndef HAD_GLOBALS_H
#define HAD_GLOBALS_H

#include "dbl.h"
#include "map.h"

extern DB *db;

extern map_t *needed_map;
extern map_t *extra_file_map;
extern map_t *extra_disk_map;

extern int romhashtypes, diskhashtypes;

extern int output_options;
extern int fix_options;
/* XXX: replace by above */
extern int fix_do, fix_print, fix_keep_long, fix_keep_unused,
    fix_keep_unknown;

#endif /* globals.h */
