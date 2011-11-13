/*
  output_datafile_xml.c -- write games to datafile.dtd XML files
  Copyright (C) 2011 Dieter Baron and Thomas Klausner

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

#include "error.h"
#include "output.h"
#include "xmalloc.h"



struct output_context_xml {
    output_context_t output;

    FILE *f;
    char *fname;
};

typedef struct output_context_xml output_context_xml_t;



static int output_datafile_xml_close(output_context_t *);
static int output_datafile_xml_game(output_context_t *, game_t *);
static int output_datafile_xml_header(output_context_t *, dat_entry_t *);



output_context_t *
output_datafile_xml_new(const char *fname)
{
    output_context_xml_t *ctx;
    FILE *f;

    ctx = (output_context_xml_t *)xmalloc(sizeof(*ctx));

    if (fname == NULL) {
	f = stdout;
	fname = "*stdout*";
    }
    else {
	if ((f=fopen(fname, "w")) == NULL) {
	    myerror(ERRDEF, "cannot create `%s': %s", fname, strerror(errno));
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

    return (output_context_t *)ctx;
}



static int
output_datafile_xml_close(output_context_t *out)
{
    output_context_xml_t *ctx;
    int ret;

    ctx = (output_context_xml_t *)out;

    if (ctx->f == NULL || ctx->f == stdout)
	ret = 0;
    else {
	fputs("</datafile>", ctx->f);
	ret = fclose(ctx->f);
    }

    free(ctx);

    return ret;
}



static int
output_datafile_xml_game(output_context_t *out, game_t *g)
{
    output_context_xml_t *ctx;
    file_t *r;
    disk_t *d;
    int i;
    game_t *parent;
    char *fl;

    ctx = (output_context_xml_t *)out;

    fprintf(ctx->f, "\t<game name=\"%s\"", game_name(g));
    output_cond_print_string(ctx->f, " cloneof=\"", game_cloneof(g, TYPE_ROM, 0), "\"");
    output_cond_print_string(ctx->f, " sampleof=\"", game_cloneof(g, TYPE_SAMPLE, 0), "\"");
    fputs(">\n", ctx->f);
    /* description is actually required */
    fprintf(ctx->f, "\t\t<description>%s</description>\n",
	    game_description(g) ? game_description(g) : game_name(g));
    for (i=0; i<game_num_files(g, TYPE_ROM); i++) {
	r = game_file(g, TYPE_ROM, i);
	
	fprintf(ctx->f, "\t\t<rom name=\"%s\"", file_name(r));
	fprintf(ctx->f, " size=\"%" PRIu64 "\"", file_size(r));
	output_cond_print_hash(ctx->f, " crc=\"", HASHES_TYPE_CRC, file_hashes(r), "\"");
	output_cond_print_hash(ctx->f, " sha1=\"", HASHES_TYPE_SHA1, file_hashes(r), "\"");
	output_cond_print_hash(ctx->f, " md5=\"", HASHES_TYPE_MD5, file_hashes(r), "\"");
	if (file_where(r) != FILE_INZIP)
	    output_cond_print_string(ctx->f, " merge=\"",
			 file_merge(r) ? file_merge(r) : file_name(r),
			 "\"");
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
	output_cond_print_string(ctx->f, " status=\"", fl, "\"");
	fputs("/>\n", ctx->f);
    }
    for (i=0; i<game_num_disks(g); i++) {
	d = game_disk(g, i);

	fprintf(ctx->f, "\t\t<disk name=\"%s\"", disk_name(d));
	output_cond_print_hash(ctx->f, " sha1=\"", HASHES_TYPE_SHA1, disk_hashes(d), "\"");
	output_cond_print_hash(ctx->f, " md5=\"", HASHES_TYPE_MD5, disk_hashes(d), "\"");
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
	output_cond_print_string(ctx->f, " status=\"", fl, "\"");
	fputs("/>\n", ctx->f);
    }
    for (i=0; i<game_num_files(g, TYPE_SAMPLE); i++) {
	r = game_file(g, TYPE_SAMPLE, i);
	fprintf(ctx->f, "\t\t<sample name=\"%s\"/>\n", file_name(r));
    }
    fputs("\t</game>\n\n", ctx->f);

    return 0;
}



static int
output_datafile_xml_header(output_context_t *out, dat_entry_t *dat)
{
    output_context_xml_t *ctx;

    ctx = (output_context_xml_t *)out;

    fprintf(ctx->f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    "<!DOCTYPE datafile PUBLIC \"-//Logiqx//DTD ROM Management Datafile//EN\" \"http://www.logiqx.com/Dats/datafile.dtd\">\n"
	    "<datafile>\n"
	    "\t<header>\n"
	    "\t\t<name>%s</name>\n"
	    "\t\t<description>%s</description>\n"
	    "\t\t<version>%s</version>\n"
	    "\t\t<author>automatically generated</author>\n"
	    "\t</header>\n\n", dat_entry_name(dat),
	    (dat_entry_description(dat)
	     ? dat_entry_description(dat) : dat_entry_name(dat)), 
	    dat_entry_version(dat));

    return 0;
}
