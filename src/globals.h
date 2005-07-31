#ifndef HAD_GLOBALS_H
#define HAD_GLOBALS_H

#include "dbl.h"
#include "delete_list.h"
#include "map.h"
#include "parray.h"

extern DB *db;

extern char *rompath[];
extern char *need_dir;

extern map_t *extra_disk_map;
extern map_t *extra_file_map;
extern map_t *needed_map;
extern delete_list_t *needed_delete_list;

extern parray_t *superfluous;

extern int romhashtypes, diskhashtypes;

extern filetype_t file_type;

extern int output_options;
extern int fix_options;

#endif /* globals.h */
