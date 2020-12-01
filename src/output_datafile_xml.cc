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
static int output_datafile_xml_game(output_context_t *, GamePtr);
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
set_attribute(xmlNodePtr node, const std::string &name, const std::string &value) {
    if (value.empty()) {
        return;
    }
    xmlSetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()), reinterpret_cast<const xmlChar *>(value.c_str()));
}

static void
set_attribute_u64(xmlNodePtr node, const char *name, uint64_t value) {
    char b[128];
    snprintf(b, sizeof(b), "%" PRIu64, value);
    set_attribute(node, name, b);
}

static void
set_attribute_hash(xmlNodePtr node, const char *name, int type, Hashes *hashes) {
    set_attribute(node, name, hashes->to_string(type));
}

static int
output_datafile_xml_game(output_context_t *out, GamePtr game) {
    auto ctx = reinterpret_cast<output_context_xml_t *>(out);
    
    xmlNodePtr xmlGame = xmlNewChild(ctx->root, NULL, reinterpret_cast<const xmlChar *>("game"), NULL);
    
    set_attribute(xmlGame, "name", game->name);
    set_attribute(xmlGame, "cloneof", game->cloneof[0]);
    /* description is actually required */
    xmlNewTextChild(xmlGame, NULL, reinterpret_cast<const xmlChar *>("description"), reinterpret_cast<const xmlChar *>(!game->description.empty() ? game->description.c_str() : game->name.c_str()));

    for (size_t i = 0; i < game->roms.size(); i++) {
        auto &rom = game->roms[i];
        xmlNodePtr xmlRom = xmlNewChild(xmlGame, NULL, reinterpret_cast<const xmlChar *>("rom"), NULL);
        
        set_attribute(xmlRom, "name", rom.name);
        set_attribute_u64(xmlRom, "size", rom.size);
        set_attribute_hash(xmlRom, "crc", Hashes::TYPE_CRC, &rom.hashes);
        set_attribute_hash(xmlRom, "sha1", Hashes::TYPE_SHA1, &rom.hashes);
        set_attribute_hash(xmlRom, "md5", Hashes::TYPE_MD5, &rom.hashes);

        if (rom.where != FILE_INGAME) {
            set_attribute(xmlRom, "merge", rom.merge.empty() ? rom.name : rom.merge);
        }

        std::string fl;
        switch (rom.status) {
            case STATUS_BADDUMP:
                fl = "baddump";
                break;

            case STATUS_NODUMP:
                fl = "nodump";
                break;
                
            default:
                break;
        }
        set_attribute(xmlRom, "status", fl);
    }

    for (size_t i = 0; i < game->disks.size(); i++) {
        auto d = &game->disks[i];
        xmlNodePtr disk = xmlNewChild(xmlGame, NULL, reinterpret_cast<const xmlChar *>("disk"), NULL);

        set_attribute(disk, "name", disk_name(d));
        set_attribute_hash(disk, "sha1", Hashes::TYPE_SHA1, disk_hashes(d));
        set_attribute_hash(disk, "md5", Hashes::TYPE_MD5, disk_hashes(d));

        std::string fl;
        
        switch (disk_status(d)) {
	case STATUS_OK:
	    fl = "";
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
