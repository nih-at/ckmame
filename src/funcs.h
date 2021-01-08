#ifndef _HAD_FUNCS_H
#define _HAD_FUNCS_H

/*
  funcs.h -- tree functions
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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

#include <string>

#include <zip.h>

#include "archive.h"
#include "delete_list.h"
#include "game.h"
#include "parray.h"
#include "result.h"
#include "tree.h"

#define DO_MAP 0x1
#define DO_LIST 0x2

#define CLEANUP_NEEDED 0x1
#define CLEANUP_UNKNOWN 0x2


void check_archive(ArchivePtr, const char *, Result *);
void check_disks(Game *, ImagesPtr [], Result *);
void check_files(Game *, ArchivePtr [], Result *);
void check_images(Images *, const char *, Result *);
void check_old(Game *, Result *);
void cleanup_list(parray_t *, delete_list_t *, int);
int copy_file(const char *, const char *, size_t, ssize_t, Hashes *);
void diagnostics(const Game *, const ArchivePtr, const Images *, const Result *);
void diagnostics_archive(const ArchivePtr, const Result *);
void diagnostics_images(const Images *, const Result *);
bool ensure_dir(const std::string &name, bool strip_filename);
void ensure_extra_maps(int);
void ensure_needed_maps(void);
bool enter_disk_in_map(const Disk *, where_t);
std::string findfile(const std::string &name, filetype_t ft, const std::string &game_name);
int fix_game(Game *, Archive *, Images *, Result *);
parray_t *list_directory(const char *, const char *);
const char *get_directory(void);
int link_or_copy(const char *, const char *);
std::string make_file_name(filetype_t, const std::string &name, const std::string &game_name);
std::string make_garbage_name(const std::string &, int);
char *make_needed_name(const File *);
char *make_needed_name_disk(const Disk *);
char *make_unique_name(const char *, const char *, ...);
int move_image_to_garbage(const std::string &fname);
int my_remove(const char *name);
int my_zip_rename(struct zip *, uint64_t, const char *);
int my_zip_rename_to_unique(struct zip *, zip_uint64_t);
int name_is_zip(const char *);
void print_superfluous(const parray_t *);
void remove_empty_archive(const char *);
void remove_from_superfluous(const char *);
int rename_or_move(const char *, const char *);
bool save_needed(Archive *sa, int sidx, const char *gamename);
int save_needed_disk(const char *, int);
bool save_needed_part(Archive *sa, int sidx, const char *gamename, off_t start, off_t length, File *f);
void write_fixdat_entry(const Game *, const Result *);

#endif /* funcs.h */
