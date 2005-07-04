#ifndef _HAD_DBH_H
#define _HAD_DBH_H

/*
  $NiH: dbh.h,v 1.9 2005/06/20 16:16:04 wiz Exp $

  dbh.h -- high level db functions
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

#include "dbl.h"
#include "types.h"



struct game *r_game(DB *, const char *);
int r_hashtypes(DB *, int *, int *);
int r_list(DB *, const char *, char ***);
int r_prog(DB *, char **, char **);
struct file_by_hash *r_file_by_hash(DB *, enum filetype,
				    const struct hashes *);
int w_game(DB *, const struct game *);
int w_hashtypes(DB *, int, int);
int w_list(DB *, const char *, const char * const *, int);
int w_prog(DB *, const char *, const char *);
int w_file_by_hash(DB *, const struct file_by_hash *);

#endif /* dbh.h */
