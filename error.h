#ifndef _HAD_ERROR_H
#define _HAD_ERROR_H

/*
  $NiH: error.h,v 1.3 2002/06/06 09:26:54 dillo Exp $

  error.h -- error printing
  Copyright (C) 1999, 2003 Dieter Baron and Thomas Klaunser

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



#define ERRDEF	0x0	/* no additional info */
#define ERRZIP	0x1	/* prepend zipflie name */
#define ERRFILE	0x2	/* prepend file name */
#define ERRSTR	0x4	/* append strerror(errno) */

#define ERRZIPFILE	(ERRZIP|ERRFILE)
#define	ERRZIPSTR	(ERRZIP|ERRSTR)
#define ERRFILESTR	(ERRFILE|ERRSTR)
#define ERRZIPFILESTR	(ERRZIPFILE|ERRSTR)

void myerror(int errtype, char *fmt, ...);
void seterrinfo(char *fn, char *zipn);

#endif /* error.h */
