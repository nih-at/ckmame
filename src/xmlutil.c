/*
  $NiH$

  xmlutil.c -- parse XML file via callbacks
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

