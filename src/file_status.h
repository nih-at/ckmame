#ifndef _HAD_FILE_STATUS_H
#define _HAD_FILE_STATUS_H

/*
  $NiH: file_status.h,v 1.1.2.2 2005/07/30 12:24:29 dillo Exp $

  file_status.h -- information about status of a file in an archive
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



#include "array.h"
#include "types.h"



typedef array_t file_status_array_t;

#define file_status_array_free(ma)	(array_free(ma, NULL))
#define file_status_array_get(ma, i)	\
	(*(file_status_t *)array_get((ma), (i)))
#define file_status_array_new(n)	\
	(array_new_length(sizeof(file_status_t), (n), file_status_init))
#define file_status_array_length	array_length



void file_status_init(file_status_t *);

#endif /* file_status.h */
