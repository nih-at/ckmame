#ifndef HAD_HASHES_H
#define HAD_HASHES_H

/*
  $NiH: hashes.h,v 1.1 2005/06/20 16:16:04 wiz Exp $

  hashes.h -- hash related functions
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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



#include "types.h"

char *hash_to_string(int, const struct hashes *);
const char *hash_type_string(int);

#endif /* hashes.h */

