#ifndef _HAD_FUNCS_H
#define _HAD_FUNCS_H

/*
  $NiH: funcs.h,v 1.9 2002/06/06 09:26:55 dillo Exp $

  funcs.h -- tree functions
  Copyright (C) 1999 Dieter Baron and Thomas Klausner

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



int tree_add(DB *db, struct tree *tree, char *name, int sample);
struct tree *tree_new(char *name, int check);
void tree_free(struct tree *tree);
void tree_traverse(DB *db, struct tree *tree, int sample);

#endif /* funcs.h */
