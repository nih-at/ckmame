#ifndef HAD_INTSTR_H
#define HAD_INTSTR_H

/*
  $NiH$

  intstr.h -- map int to strings and back
  Copyright (C) 2007 Dieter Baron and Thomas Klausner

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



struct intstr {
    int i;
    const char *s;
};

typedef struct intstr intstr_t;



const char *int2str(int, const intstr_t *);
int str2int(const char *, const intstr_t *);

#endif /*intstr.h */
