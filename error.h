#ifndef _HAD_ERROR_H
#define _HAD_ERROR_H

/*
  $NiH$

  error.h -- error printing
  Copyright (C) 1999 Dieter Baron and Thomas Klaunser

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



/* error types: default error message (no filename), */
#define ERRDEF    0
/* or file error, which also outputs filename (and zipfilename), */
#define ERRFILE   1
/* or zipfile error, which only outputs the zipfilename, */
#define ERRZIP    2
/* or file error with additional strerror(errno) output, */
#define ERRSTR    3
/* or zipfile error with additional strerror(errno) output */
#define ERRZIPSTR 4

void myerror(int errtype, char *fmt, ...);
void seterrinfo(char *fn, char *zipn);

#endif /* error.h */
