#ifndef HAD_XMLUTIL_H
#define HAD_XMLUTIL_H

/*
  $NiH$

  xmlutil.h -- parse XML file via callbacks
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



#include <stdbool.h>
#include <stdio.h>

typedef int (*xmlu_attr_cb)(void *, int, int, const char *);
typedef int (*xmlu_tag_cb)(void *, int);
typedef int (*xmlu_text_cb)(void *, const char *);

struct xmlu_attr {
    const char *name;
    xmlu_attr_cb cb_attr;
    int arg1;
    int arg2;
};

typedef struct xmlu_attr xmlu_attr_t;

struct xmlu_entity {
    const char *name;
    const xmlu_attr_t *attr;
    bool empty;
    xmlu_tag_cb cb_open;
    xmlu_tag_cb cb_close;
    xmlu_text_cb cb_text;
    int arg1;
};

typedef struct xmlu_entity xmlu_entity_t;



int xmlu_parse(FILE *, void *, const xmlu_entity_t *, int);


#endif /* xmlutil.h */
