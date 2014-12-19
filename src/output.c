/*
  output.c -- output game info
  Copyright (C) 2006-2013 Dieter Baron and Thomas Klausner

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


#include "error.h"
#include "output.h"


int
output_close(output_context_t *ctx)
{
    return ctx->close(ctx);
}


int
output_detector(output_context_t *ctx, detector_t *detector)
{
    if (ctx->output_detector == NULL)
	return 0;

    return ctx->output_detector(ctx, detector);
}


int
output_game(output_context_t *ctx, game_t *g)
{
    return ctx->output_game(ctx, g);
}


int
output_header(output_context_t *ctx, dat_entry_t *de)
{
    if (ctx->output_header == NULL)
	return 0;

    return ctx->output_header(ctx, de);
}


output_context_t *
output_new(output_format_t fmt, const char *fname, int flags)
{
    switch (fmt) {
    case OUTPUT_FMT_CM:
	return output_cm_new(fname, flags);
    case OUTPUT_FMT_DB:
	return output_db_new(fname, flags);
    case OUTPUT_FMT_DATAFILE_XML:
	return output_datafile_xml_new(fname, flags);
    case OUTPUT_FMT_MTREE:
	return output_mtree_new(fname, flags);
    default:
	return NULL;
    }
}


void
output_cond_print_string(FILE *f, const char *pre, const char *str,
			 const char *post)
{
    char *q;

    if (str == NULL)
	return;

    if (strcspn(str, " \t") == strlen(str))
	q = "";
    else
	q = "\"";

    fprintf(f, "%s%s%s%s%s", pre, q, str, q, post);
}


void
output_cond_print_hash(FILE *f, const char *pre, int t, hashes_t *h, const char *post)
{
    char hstr[HASHES_SIZE_MAX*2+1];

    output_cond_print_string(f, pre, hash_to_string(hstr, t, h), post);
}
