#ifndef _HAD_DBL_INT_H
#define _HAD_DBL_INT_H
   
/*
  $NiH: db-db.h,v 1.8 2003/02/23 14:48:04 dillo Exp $

  db-db.h -- low level routines for Berkley db 
  Copyright (C) 1999, 2003 Dieter Baron and Thomas Klausner

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



#include "config.h"

#ifdef HAVE_DB_185_H
#include <db_185.h>
#else
#include <db.h>
#endif
#define DDB_FILEEXT ".db"

#endif
