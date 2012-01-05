#ifndef _HAD_FUNCS_H
#define _HAD_FUNCS_H

/*
  funcs.h -- tree functions
  Copyright (C) 1999-2011 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include <zip.h>

#include "archive.h"
#include "delete_list.h"
#include "game.h"
#include "parray.h"
#include "result.h"
#include "tree.h"

#define DO_MAP  0x1
#define DO_LIST	0x2

#define CLEANUP_NEEDED	0x1
#define CLEANUP_UNKNOWN	0x2



void check_archive(archive_t *, const char *, result_t *);
void check_disks(game_t *, images_t *, result_t *);
void check_files(game_t *, archive_t *[], result_t *);
void check_images(images_t *, const char *, result_t *);
void check_old(game_t *, result_t *);
void cleanup_list(parray_t *, delete_list_t *, int);
int copy_file(const char *, const char *);
void diagnostics(const game_t *, const archive_t *, const images_t *,
		 const result_t *);
void diagnostics_archive(const archive_t *, const result_t *);
void diagnostics_images(const images_t *, const result_t *);
int ensure_dir(const char *, int);
void ensure_extra_maps(int);
void ensure_needed_maps(void);
int enter_disk_in_map(const disk_t *, where_t);
int enter_file_in_map(const archive_t *, int, where_t);
char *findfile(const char *, filetype_t);
int fix_game(game_t *, archive_t *, images_t *, result_t *);
parray_t *list_directory(const char *, const char *);
const char *get_directory(filetype_t);
int link_or_copy(const char *, const char *);
char *make_file_name(filetype_t, int, const char *);
char *make_garbage_name(const char *, int);
char *make_needed_name(const file_t *);
char *make_needed_name_disk(const disk_t *);
char *make_unique_name(const char *, const char *, ...);
int move_image_to_garbage(const char *);
int my_remove(const char *name);
struct zip *my_zip_open(const char *, int);
int my_zip_rename(struct zip *, int, const char *);
int my_zip_rename_to_unique(struct zip *, int);
int name_is_zip(const char *);
void print_superfluous(const parray_t *);
void remove_empty_archive(const char *);
void remove_from_superfluous(const char *);
int rename_or_move(const char *, const char *);
int save_needed(archive_t *, int, int);
int save_needed_disk(const char *, int);

#endif /* funcs.h */
