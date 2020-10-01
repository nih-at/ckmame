/*
  xmlutil.c -- parse XML file via callbacks
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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
xmlu_parse(parser_source_t *ps, void *ctx, xmlu_lineno_cb lineno_cb, const xmlu_entity_t *entities, int nentities) {
    myerror(ERRFILE, "support for XML parsing not compiled in.");
    return -1;
}

#else

#include "parse.h"
#include <libxml/xmlreader.h>

#define XMLU_MAX_PATH 8192

static const xmlu_entity_t *find_entity(const char *, const xmlu_entity_t *, int);
static int xml_close(void *);
static int xml_read(void *, char *, int);


int
xmlu_parse(parser_source_t *ps, void *ctx, xmlu_lineno_cb lineno_cb, const xmlu_entity_t *entities, int nentities) {
    xmlTextReaderPtr reader;
    int i, ret;
    const char *name;
    char path[XMLU_MAX_PATH];
    char *attr;
    const xmlu_entity_t *e, *e_txt;
    const xmlu_attr_t *a;

    reader = xmlReaderForIO(xml_read, xml_close, ps, NULL, NULL, 0);
    if (reader == NULL) {
	/* TODO */
	printf("opening error\n");
	return -1;
    }

    e_txt = NULL;
    path[0] = '\0';

    while ((ret = xmlTextReaderRead(reader)) == 1) {
	if (lineno_cb)
	    lineno_cb(ctx, xmlTextReaderGetParserLineNumber(reader));

	switch (xmlTextReaderNodeType(reader)) {
	case XML_READER_TYPE_ELEMENT:
	    name = (const char *)xmlTextReaderConstName(reader);
	    if (path + strlen(path) + strlen(name) + 2 > path + sizeof(path)) {
		/* TODO */
                xmlFreeTextReader(reader);
                printf("element path too long\n");
                return -1;
	    }
	    else {
		sprintf(path + strlen(path), "/%s", name);
	    }

	    if ((e = find_entity(path, entities, nentities)) != NULL) {
		if (e->cb_open)
		    ret |= e->cb_open(ctx, e->arg1);

		a = e->attr;
		for (i = 0; a && a[i].name; i++) {
		    if ((attr = (char *)xmlTextReaderGetAttribute(reader, (const xmlChar *)e->attr[i].name)) != NULL) {
			ret |= a[i].cb_attr(ctx, a[i].arg1, a[i].arg2, attr);
			free(attr);
		    }
		}

		if (e->cb_text)
		    e_txt = e;
	    }

	    if (!xmlTextReaderIsEmptyElement(reader))
		break;
	    /*
	      Fallthrough for empty elements, as we won't get an
	      extra close.
	    */

	case XML_READER_TYPE_END_ELEMENT:
	    if ((e = find_entity(path, entities, nentities)) != NULL) {
		if (e->cb_close)
		    ret |= e->cb_close(ctx, e->arg1);
	    }

	    *(strrchr(path, '/')) = '\0';
	    e_txt = NULL;

	    break;

	case XML_READER_TYPE_TEXT:
	    if (e_txt)
		e_txt->cb_text(ctx, (const char *)xmlTextReaderConstValue(reader));
	    break;

	default:
	    break;
	}
    }
    xmlFreeTextReader(reader);

    if (ret != 0) {
	/* TODO: parse error */
	printf("parse error\n");
	return -1;
    }

    return 0;
}


static const xmlu_entity_t *
find_entity(const char *path, const xmlu_entity_t *entities, int nentities) {
    int i;
    const char *path_end;
    size_t name_len;

    path_end = path + strlen(path);

    for (i = 0; i < nentities; i++) {
	if (entities[i].name[0] == '/') {
	    if (strcmp(path, entities[i].name) == 0)
		break;
	}
	else {
	    name_len = strlen(entities[i].name);
	    if (path_end[-name_len - 1] == '/' && strcmp(path_end - name_len, entities[i].name) == 0)
		break;
	}
    }

    if (i != nentities)
	return entities + i;

    return NULL;
}


static int
xml_close(void *ctx) {
    return 0;
}


static int
xml_read(void *ctx, char *b, int len) {
    return (int)ps_read(static_cast<parser_source_t *>(ctx), b, len);
}

#endif /* HAVE_LIBXML2 */
