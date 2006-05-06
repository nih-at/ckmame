#ifndef HAD_GLOBALS_H
#define HAD_GLOBALS_H

#include "dbl.h"
#include "delete_list.h"
#include "map.h"
#include "parray.h"

extern DB *db;
extern DB *old_db;

extern char *rompath[];
extern char *needed_dir;
extern char *unknown_dir;

extern parray_t *search_dirs;

extern delete_list_t *extra_delete_list;
extern map_t *extra_disk_map;
extern map_t *extra_file_map;
extern parray_t *extra_list;
extern delete_list_t *needed_delete_list;
extern map_t *needed_disk_map;
extern map_t *needed_file_map;
extern delete_list_t *superfluous_delete_list;

extern parray_t *superfluous;

extern int diskhashtypes;	/* hash type recorded for disks in db */
extern int romhashtypes;	/* hash type recorded for ROMs in db */
extern int check_integrity;	/* full integrity check of ROM set */

extern filetype_t file_type;	/* type of files to check (ROMs or samples) */

extern int output_options;
extern int fix_options;

#endif /* globals.h */
