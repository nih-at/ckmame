/*
  output_datafile_xml.c -- write games to datafile.dtd XML files
  Copyright (C) 2011-2014 Dieter Baron and Thomas Klausner

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


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/tree.h>

#include "error.h"
#include "output.h"
#include "xmalloc.h"


struct output_context_xml {
    output_context_t output;

    xmlDocPtr doc;
    xmlNodePtr root;
    FILE *f;
    char *fname;
};

typedef struct output_context_xml output_context_xml_t;


static int output_datafile_xml_close(output_context_t *);
static int output_datafile_xml_game(output_context_t *, game_t *);
static int output_datafile_xml_header(output_context_t *, dat_entry_t *);


output_context_t *
output_datafile_xml_new(const char *fname, int flags) {
    output_context_xml_t *ctx;
    FILE *f;

    ctx = (output_context_xml_t *)xmalloc(sizeof(*ctx));

    if (fname == NULL) {
	f = stdout;
	fname = "*stdout*";
    }
    else {
	if ((f = fopen(fname, "w")) == NULL) {
	    myerror(ERRDEF, "cannot create '%s': %s", fname, strerror(errno));
	    free(ctx);
	    return NULL;
	}
    }

    ctx->output.close = output_datafile_xml_close;
    ctx->output.output_detector = NULL;
    ctx->output.output_game = output_datafile_xml_game;
    ctx->output.output_header = output_datafile_xml_header;

    ctx->f = f;
    ctx->fname = xstrdup(fname);
    ctx->doc = xmlNewDoc((const xmlChar *)"1.0");
    ctx->doc->encoding = (const xmlChar *)strdup("UTF-8");
    xmlCreateIntSubset(ctx->doc, (const xmlChar *)"datafile", (const xmlChar *)"-//Logiqx//DTD ROM Management Datafile//EN", (const xmlChar *)"http://www.logiqx.com/Dats/datafile.dtd");
    ctx->root = xmlNewNode(NULL, (const xmlChar *)"datafile");
    xmlDocSetRootElement(ctx->doc, ctx->root);

    return (output_context_t *)ctx;
}


static int
output_datafile_xml_close(output_context_t *out) {
    output_context_xml_t *ctx = (output_context_xml_t *)out;

    int ret = 0;
    if (ctx->f != NULL) {
        if (xmlDocFormatDump(ctx->f, ctx->doc, 1) < 0) {
            ret = -1;
        }
	if (ctx->f != stdout) {
	    ret |= fclose(ctx->f);
	}
    }

    xmlFreeDoc(ctx->doc);
    free(ctx);

    return ret;
}

static void
set_attribute(xmlNodePtr node, const char *name, const char *value) {
    if (value == NULL) {
        return;
    }
    xmlSetProp(node, (const xmlChar *)name, (const xmlChar *)value);
}

static void
set_attribute_u64(xmlNodePtr node, const char *name, uint64_t value) {
    char b[128];
    snprintf(b, sizeof(b), "%" PRIu64, value);
    set_attribute(node, name, b);
}

static void
set_attribute_hash(xmlNodePtr node, const char *name, int type, hashes_t *hashes) {
    char hstr[HASHES_SIZE_MAX * 2 + 1];
    
    set_attribute(node, name, hash_to_string(hstr, type, hashes));
}

static int
output_datafile_xml_game(output_context_t *out, game_t *g) {
    output_context_xml_t *ctx;
    file_t *r;
    disk_t *d;
    int i;
    char *fl;

    ctx = (output_context_xml_t *)out;
    
    xmlNodePtr game = xmlNewChild(ctx->root, NULL, (const xmlChar *)"game", NULL);
    
    set_attribute(game, "name", game_name(g));
    set_attribute(game, "cloneof", game_cloneof(g, 0));
    /* description is actually required */
    xmlNewTextChild(game, NULL, (const xmlChar *)"description", (const xmlChar *)(game_description(g) ? game_description(g) : game_name(g)));

    for (i = 0; i < game_num_files(g); i++) {
	r = game_file(g, i);
        xmlNodePtr rom = xmlNewChild(game, NULL, (const xmlChar *)"rom", NULL);
        
        set_attribute(rom, "name", file_name(r));
        set_attribute_u64(rom, "size", file_size(r));
        set_attribute_hash(rom, "crc", HASHES_TYPE_CRC, file_hashes(r));
        set_attribute_hash(rom, "sha1", HASHES_TYPE_SHA1, file_hashes(r));
        set_attribute_hash(rom, "md5", HASHES_TYPE_MD5, file_hashes(r));

        if (file_where(r) != FILE_INZIP) {
            set_attribute(rom, "merge", file_merge(r) ? file_merge(r) : file_name(r));
        }

        switch (file_status(r)) {
	case STATUS_OK:
	    fl = NULL;
	    break;
	case STATUS_BADDUMP:
	    fl = "baddump";
	    break;
	case STATUS_NODUMP:
	    fl = "nodump";
	    break;
	}
        set_attribute(rom, "status", fl);
    }

    for (i = 0; i < game_num_disks(g); i++) {
	d = game_disk(g, i);
        xmlNodePtr disk = xmlNewChild(game, NULL, (const xmlChar *)"disk", NULL);

        set_attribute(disk, "name", disk_name(d));
        set_attribute_hash(disk, "sha1", HASHES_TYPE_SHA1, disk_hashes(d));
        set_attribute_hash(disk, "md5", HASHES_TYPE_MD5, disk_hashes(d));

        switch (disk_status(d)) {
	case STATUS_OK:
	    fl = NULL;
	    break;
	case STATUS_BADDUMP:
	    fl = "baddump";
	    break;
	case STATUS_NODUMP:
	    fl = "nodump";
	    break;
	}
        set_attribute(disk, "status", fl);
    }
    
    return 0;
}


static int
output_datafile_xml_header(output_context_t *out, dat_entry_t *dat) {
    output_context_xml_t *ctx;

    ctx = (output_context_xml_t *)out;
    
    xmlNodePtr header = xmlNewChild(ctx->root, NULL, (const xmlChar *)"header", NULL);
    
    xmlNewTextChild(header, NULL, (const xmlChar *)"name", (const xmlChar *)dat_entry_name(dat));
    xmlNewTextChild(header, NULL, (const xmlChar *)"description", (const xmlChar *)(dat_entry_description(dat) ? dat_entry_description(dat) : dat_entry_name(dat)));
    xmlNewTextChild(header, NULL, (const xmlChar *)"version", (const xmlChar *)dat_entry_version(dat));
    xmlNewTextChild(header, NULL, (const xmlChar *)"author", (const xmlChar *)"automatically generated");

    return 0;
}
