#ifndef _HAD_ROMUTIL_H
#define _HAD_ROMUTIL_H

/*
  romutil.h -- miscellaneous utility functions for rom handling
  Copyright (C) 1999 Dieter Baron and Thomas Klaunser

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



/* not all of these are in romutil.c */

enum state romcmp(struct rom *r1, struct rom *r2, int merge);
struct zip *zip_new(char *name);
void marry (struct match *rm, int count, int *noz);
struct match *check_game(struct game *game, struct zip **zip,
			 int pno, int gpno);
int matchcmp(struct match *m1, struct match *m2);
void diagnostics(struct game *game, struct match *m, struct zip **zip);
void match_free(struct match *m, int n);
int countunused(struct zip *z);
char *findzip(char *name);
void game_free(struct game *g, int fullp);
char **delchecked(struct tree *t, int nclone, char **clone);
void zip_free(struct zip *zip);
int readinfosfromzip (struct rom **rompp, char *zipfile);
void merge_match(struct match *m, int nrom, struct zip **zip,
		 int pno, int gpno);

#endif
