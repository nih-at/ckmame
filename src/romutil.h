#ifndef _HAD_ROMUTIL_H
#define _HAD_ROMUTIL_H

/*
  $NiH: romutil.h,v 1.19 2005/06/20 16:16:04 wiz Exp $

  romutil.h -- miscellaneous utility functions for rom handling
  Copyright (C) 1999, 2004 Dieter Baron and Thomas Klausner

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



/* not all of these are in romutil.c */

char **delchecked(const struct tree *, int, const char * const *);
void game_free(struct game *, int);
void game_swap_rs(struct game *);
int hashes_cmp(const struct hashes *, const struct hashes *);
void hashes_init(struct hashes *);
void marry(struct match *, int, const int *);
int read_infos_from_chd(struct disk *, int);
int read_infos_from_zip(struct zfile *, int);
void rom_add_name(struct rom *, const char *);
enum state romcmp(const struct rom *, const struct rom *, int);
struct zfile *zfile_new(const char *, int, const char *);

struct match *check_game(struct game *game, struct zfile **zip);
int matchcmp(struct match *m1, struct match *m2);
void diagnostics(struct game *, struct match *, struct disk_match *,
		 struct zfile **);
void match_free(struct match *m, int n);
int countunused(struct zfile *z);
int zfile_free(struct zfile *zip);
void merge_match(struct match *m, int nrom, struct zfile **zip,
		 int pno, int gpno);
int findcrc(struct zfile *zip, int idx, int romsize, const struct hashes *);
int fix_game(struct game *g, struct zfile **zip, struct match *m);

void disk_match_free(struct disk_match *, int);
struct disk_match *check_disks(struct game *);

void file_by_hash_free(struct file_by_hash *);
struct file_by_hash *file_by_hash_new(enum filetype, const struct hashes *);
char *file_by_hash_make_key(enum filetype, const struct hashes *);

#endif
