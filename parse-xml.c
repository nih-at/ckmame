#include <stdio.h>
#include <string.h>

#include "config.h"
#include "parse.h"

#ifndef HAVE_LIBXML2
#include "error.h"

int
parse_xml(FILE *f)
{
    myerror(ERRFILE, "support for XML parsing not compiled in.");
    return -1;
}
#else

#include <libxml/xmlreader.h>

static int do_attr(xmlTextReaderPtr, const char *, int (*)(const char *));
int xml_close(void *);
int xml_read(void *, char *, int);



int
parse_xml(FILE *f)
{
    xmlTextReaderPtr reader;
    int in_description;
    int ret;
    const xmlChar *name;

    reader = xmlReaderForIO(xml_read, xml_close, f, NULL, NULL, 0);
    if (reader == NULL) {
	/* XXX */
	printf("opening error\n");
	return -1;
    }

    in_description = 0;

    while ((ret=xmlTextReaderRead(reader)) == 1) {
	name = xmlTextReaderConstName(reader);

	if (strcmp(name, "game") == 0
	    || strcmp(name, "machine") == 0) {
	    if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
		parse_game_start();
		do_attr(reader, "name", parse_game_name);
		do_attr(reader, "romof", parse_game_cloneof);
		do_attr(reader, "sampleof", parse_game_sampleof);
	    }
	    else
		parse_game_end();
	}
	else if (strcmp(name, "rom") == 0) {
	    parse_rom_start();
	    do_attr(reader, "name", parse_rom_name);
	    do_attr(reader, "size", parse_rom_size);
	    do_attr(reader, "crc", parse_rom_crc);
	    do_attr(reader, "md5", parse_rom_md5);
	    do_attr(reader, "sha1", parse_rom_sha1);
	    do_attr(reader, "merge", parse_rom_merge);
	    do_attr(reader, "status", parse_rom_flags);
	    parse_rom_end();
	}
	else if (strcmp(name, "disk") == 0) {
	    parse_disk_start();
	    do_attr(reader, "name", parse_disk_name);
	    do_attr(reader, "md5", parse_disk_md5);
	    do_attr(reader, "sha1", parse_disk_sha1);
	    parse_disk_end();
	}
	else if (strcmp(name, "sample") == 0) {
	    parse_sample_start();
	    do_attr(reader, "name", parse_sample_name);
	    parse_sample_end();
	}
	else if (strcmp(name, "description") == 0) {
	    if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT)
		in_description = 1;
	    else
		in_description = 0;
	}
	else if (in_description && strcmp(name, "#text") == 0)
	    parse_game_description(xmlTextReaderConstValue(reader));
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
do_attr(xmlTextReaderPtr reader, const char *name,
	int (*func)(const char *))
{
    char *attr;
    int ret;

    if ((attr=xmlTextReaderGetAttribute(reader, name)) == NULL)
	return 1;

    ret = func(attr);
    free(attr);
    return ret;
}



int
xml_close(void *ctx)
{
    return 0;
}



int
xml_read(void *ctx, char *b, int len)
{
    return fread(b, 1, len, (FILE *)ctx);
}

#endif /* HAVE_LIBXML2 */

