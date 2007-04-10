/*
  $NiH$

  xmlutil.c -- parse XML file via callbacks
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



#include <stdio.h>
#include <string.h>

#include "config.h"
#include "xmlutil.h"

#ifndef HAVE_LIBXML2

#include "error.h"

/*ARGSUSED1*/
int
xmlu_parse(FILE *fin, void *ctx, const xmlu_entity_t *entities, int nentities)
{
    myerror(ERRFILE, "support for XML parsing not compiled in.");
    return -1;
}

#else

#include <libxml/xmlreader.h>

static int entity_cmp(const void *, const void *);
static int xml_close(void *);
static int xml_read(void *, char *, int);



int
xmlu_parse(FILE *fin, void *ctx, const xmlu_entity_t *entities, int nentities)
{
    xmlTextReaderPtr reader;
    int i, ret;
    const char *name;
    char *attr;
    const xmlu_entity_t *e, *e_txt;
    const xmlu_attr_t *a;

    reader = xmlReaderForIO(xml_read, xml_close, fin, NULL, NULL, 0);
    if (reader == NULL) {
	/* XXX */
	printf("opening error\n");
	return -1;
    }

    e_txt = NULL;

    while ((ret=xmlTextReaderRead(reader)) == 1) {
	name = (const char *)xmlTextReaderConstName(reader);

	if ((e=bsearch(name, entities, nentities, sizeof(entities[0]),
		       entity_cmp)) != NULL) {
	    if (e->empty
		|| xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
		if (e->cb_open)
		    ret |= e->cb_open(ctx, e->arg1);
		a = e->attr;
		for (i=0; a && a[i].name; i++) {
		    if ((attr=(char *)xmlTextReaderGetAttribute(reader,
				(const xmlChar *)e->attr[i].name)) != NULL) {
			ret |= a[i].cb_attr(ctx, a[i].arg1, a[i].arg2, attr);
			free(attr);
		    }
		}
		if (e->cb_text)
		    e_txt = e;
	    }
	    if (e->empty 
		|| xmlTextReaderNodeType(reader) != XML_READER_TYPE_ELEMENT) {
		if (e->cb_close)
		    ret |= e->cb_close(ctx, e->arg1);
		e_txt = NULL;
	    }
	}
	else if (e_txt && strcmp(name, "#text") == 0)
	    e_txt->cb_text(ctx, (const char *)xmlTextReaderConstValue(reader));
    }
    xmlFreeTextReader(reader);

    if (ret != 0) {
	/* XXX: parse error */
	printf("parse error\n");
	return -1;
    }

    return 0;
}



static int
entity_cmp(const void *key, const void *ve)
{
    return strcmp(key, ((const xmlu_entity_t *)ve)->name);
}



static int
xml_close(void *ctx)
{
    return 0;
}



static int
xml_read(void *ctx, char *b, int len)
{
    return fread(b, 1, len, (FILE *)ctx);
}

#endif /* HAVE_LIBXML2 */

