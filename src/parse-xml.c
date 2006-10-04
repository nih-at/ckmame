/*
  $NiH: parse-xml.c,v 1.7 2006/05/24 09:29:18 dillo Exp $

  parse-xml.c -- parse listxml format files
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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
#include "parse.h"

#ifndef HAVE_LIBXML2

#include "error.h"

/*ARGSUSED1*/
int
parse_xml(parser_context_t *ctx)
{
    myerror(ERRFILE, "support for XML parsing not compiled in.");
    return -1;
}

#else

#include <libxml/xmlreader.h>

struct attr {
    const char *name;
    int (*fn)(parser_context_t *, filetype_t, int, const char *);
    filetype_t ft;
    int ht;
};

struct entity {
    const char *name;
    const struct attr *attr;
    int empty;
    int (*start)(parser_context_t *, filetype_t);
    int (*end)(parser_context_t *, filetype_t);
    filetype_t ft;
};

static const struct attr attr_disk[] = {
    { "md5",      parse_file_hash,    TYPE_DISK,   HASHES_TYPE_MD5  },
    { "merge",    parse_file_merge,   TYPE_DISK,   0                },
    { "name",     parse_file_name,    TYPE_DISK,   0                },
    { "sha1",     parse_file_hash,    TYPE_DISK,   HASHES_TYPE_SHA1 },
    { "status",   parse_file_status,   TYPE_DISK,   0                },
    { NULL }
};
static const struct attr attr_game[] = {
    { "name",     parse_game_name,    0,           0                },
    { "romof",    parse_game_cloneof, TYPE_ROM,    0                },
    { "sampleof", parse_game_cloneof, TYPE_SAMPLE, 0                },
    { NULL }
};
static const struct attr attr_rom[] = {
    { "crc",      parse_file_hash,    TYPE_ROM,    HASHES_TYPE_CRC  },
    { "md5",      parse_file_hash,    TYPE_ROM,    HASHES_TYPE_MD5  },
    { "merge",    parse_file_merge,   TYPE_ROM,    0                },
    { "name",     parse_file_name,    TYPE_ROM,    0                },
    { "sha1",     parse_file_hash,    TYPE_ROM,    HASHES_TYPE_SHA1 },
    { "size",     parse_file_size,    TYPE_ROM,    0                },
    { "status",   parse_file_status,   TYPE_ROM,    0                },
    { NULL }
};
static const struct attr attr_sample[] = {
    { "name",     parse_file_name,    TYPE_SAMPLE, 0                },
    { NULL }
};
static const struct entity entity[] = {
    { "disk",   attr_disk,   1, parse_file_start, parse_file_end, TYPE_DISK },
    { "game",   attr_game,   0, parse_game_start, parse_game_end, 0 },
    { "machine", attr_game,  0, parse_game_start, parse_game_end, 0 },
    { "rom",    attr_rom,    1, parse_file_start, parse_file_end, TYPE_ROM },
    { "sample", attr_sample, 1, parse_file_start, parse_file_end, TYPE_SAMPLE }
};
static const int nentity = sizeof(entity)/sizeof(entity[0]);



static int entity_cmp(const void *, const void *);
static int xml_close(void *);
static int xml_read(void *, char *, int);



int
parse_xml(FILE *fin, parser_context_t *ctx)
{
    xmlTextReaderPtr reader;
    int in_description;
    int i, ret;
    const char *name;
    char *attr;
    const struct entity *e;
    const struct attr *a;

    reader = xmlReaderForIO(xml_read, xml_close, fin, NULL, NULL, 0);
    if (reader == NULL) {
	/* XXX */
	printf("opening error\n");
	return -1;
    }

    ctx->lineno = 0;

    in_description = 0;

    while ((ret=xmlTextReaderRead(reader)) == 1) {
	name = (const char *)xmlTextReaderConstName(reader);

	if ((e=bsearch(name, entity, nentity, sizeof(entity[0]),
		       entity_cmp)) != NULL) {
	    if (e->empty
		|| xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
		ret |= e->start(ctx, e->ft);
		a = e->attr;
		for (i=0; a[i].name; i++) {
		    if ((attr=(char *)xmlTextReaderGetAttribute(reader,
				(const xmlChar *)e->attr[i].name)) != NULL) {
			ret |= a[i].fn(ctx, a[i].ft, a[i].ht, attr);
			free(attr);
		    }
		}
	    }
	    if (e->empty 
		|| xmlTextReaderNodeType(reader) != XML_READER_TYPE_ELEMENT)
		ret |= e->end(ctx, e->ft);
	}
	else if (strcmp(name, "description") == 0) {
	    if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT)
		in_description = 1;
	    else
		in_description = 0;
	}
	else if (in_description && strcmp(name, "#text") == 0)
	    parse_game_description(ctx,
			   (const char *)xmlTextReaderConstValue(reader));
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
    return strcmp(key, ((const struct entity *)ve)->name);
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

