#ifndef _HAD_FUNCS_H
#define _HAD_FUNCS_H

/*
  $NiH: funcs.h,v 1.19 2006/05/06 23:31:40 dillo Exp $

  funcs.h -- tree functions
  Copyright (C) 1999, 2004, 2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <zip.h>

#include "archive.h"
#include "dbl.h"
#include "delete_list.h"
#include "game.h"
#include "parray.h"
#include "result.h"
#include "tree.h"

#define DO_MAP  0x1
#define DO_LIST	0x2

#define CLEANUP_NEEDED	0x1
#define CLEANUP_UNKNOWN	0x2

enum name_type {
    NAME_ZIP,
    NAME_CHD,
    NAME_NOEXT,
    NAME_UNKNOWN
};

typedef enum name_type name_type_t;



void check_archive(archive_t *, const char *, result_t *);
void check_disks(game_t *, images_t *, result_t *);
void check_files(game_t *, archive_t *[], result_t *);
void check_images(images_t *, const char *, result_t *);
void check_old(game_t *, result_t *);
void cleanup_list(parray_t *, delete_list_t *, int);
void diagnostics(const game_t *, const archive_t *, const images_t *,
		 const result_t *);
void diagnostics_archive(const archive_t *, const result_t *);
void diagnostics_images(const images_t *, const result_t *);
int ensure_dir(const char *, int);
void ensure_extra_maps(int);
void ensure_needed_maps(void);
char *findfile(const char *, filetype_t);
int fix_game(game_t *, archive_t *, images_t *, result_t *);
parray_t *find_superfluous(const char *);
void init_rompath(void);
char *make_file_name(filetype_t, int, const char *);
char *make_garbage_name(const char *, int);
char *make_needed_name(const rom_t *);
char *make_needed_name_disk(const disk_t *);
char *make_unique_name(const char *, const char *, ...);
int move_image_to_garbage(const char *);
int my_remove(const char *name);
struct zip *my_zip_open(const char *, int);
int my_zip_rename(struct zip *, int, const char *);
char *my_zip_unique_name(struct zip *, const char *);
name_type_t name_type(const char *);
int name_is_zip(const char *);
void print_superfluous(const parray_t *);
void remove_empty_archive(const char *);
void remove_from_superfluous(const char *);
int rename_or_move(const char *, const char *);
int save_needed(archive_t *, int, int);
int save_needed_disk(const char *, int);

#endif /* funcs.h */
