#ifndef _HAD_DBH_H
#define _HAD_DBH_H

/*
  $NiH: dbh.h,v 1.6 2004/04/21 10:38:37 dillo Exp $

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
int w_game(DB *, const struct game *);
int r_list(DB *, const char *, char ***);
int w_list(DB *, const char *, const char * const *, int);
int r_prog(DB *, char **, char **);
int w_prog(DB *, const char *, const char *);

#endif /* dbh.h */
