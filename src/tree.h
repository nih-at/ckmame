#ifndef HAD_TREE_H
#define HAD_TREE_H

/*
  $NiH: tree.h,v 1.1.2.3 2005/07/30 12:24:29 dillo Exp $

  tree.h -- XXX
  Copyright (C) 1999-2005 Dieter Baron and Thomas Klausner

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

struct tree {
    char *name;
    int check;
    struct tree *next, *child;
};

typedef struct tree tree_t;



#define tree_name(t)	((t)->name)

int tree_add(tree_t *, const char *);
void tree_free(tree_t *);
tree_t *tree_new(void);
void tree_traverse(const tree_t *, archive_t *, archive_t *);

#endif /* tree.h */
