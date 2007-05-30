#ifndef HAD_GLOBALS_H
#define HAD_GLOBALS_H

/*
  $NiH: globals.h,v 1.9 2006/10/04 17:36:44 dillo Exp $

  globals.h -- declaration of global variables
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

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



#include <sqlite3.h>

#include "delete_list.h"
#include "detector.h"
#include "parray.h"

extern sqlite3 *db;
extern sqlite3 *old_db;

extern detector_t *detector;

extern char *rompath[];
extern char *needed_dir;
extern char *unknown_dir;

extern parray_t *search_dirs;

extern delete_list_t *extra_delete_list;
extern parray_t *extra_list;
extern delete_list_t *needed_delete_list;
extern delete_list_t *superfluous_delete_list;

extern parray_t *superfluous;

extern int diskhashtypes;	/* hash type recorded for disks in db */
extern int romhashtypes;	/* hash type recorded for ROMs in db */
extern int check_integrity;	/* full integrity check of ROM set */

extern filetype_t file_type;	/* type of files to check (ROMs or samples) */

extern int output_options;
extern int fix_options;

#endif /* globals.h */
