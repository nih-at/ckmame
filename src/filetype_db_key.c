/*
  $NiH$

  filetype_db_key.c -- get db key for list of files of a type
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

#include <string.h>

#include "dbl.h"
#include "types.h"



const char *
filetype_db_key(filetype_t ft)
{
    switch (ft) {
    case TYPE_ROM:
	return DDB_KEY_LIST_GAME;

    case TYPE_SAMPLE:
	return DDB_KEY_LIST_SAMPLE;

    case TYPE_DISK:
	return DDB_KEY_LIST_DISK;

    default:
	return NULL;
    }
}