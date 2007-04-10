/*
  $NiH: output.c,v 1.3 2006/10/04 17:36:44 dillo Exp $

  output.c -- output game info
  Copyright (C) 2006-2007 Dieter Baron and Thomas Klausner

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



#include "error.h"
#include "output.h"



int
output_close(output_context_t *ctx)
{
    return ctx->close(ctx);
}



int
output_detector(output_context_t *ctx, const char *fname)
{
    detector_t *d;
    int ret;

    if (ctx->output_detector == NULL) {
	myerror(ERRDEF, "detector not supported by output format");
	return -1;
    }

    if ((d=detector_parse(fname)) == NULL) {
	myerror(ERRSTR, "cannot parse detector `%s'", fname);
	return -1;
    }

    ret = ctx->output_detector(ctx, d);

    detector_free(d);

    return ret;
}



int
output_game(output_context_t *ctx, game_t *g)
{
    return ctx->output_game(ctx, g);
}



int
output_header(output_context_t *ctx, dat_entry_t *de)
{
        return ctx->output_header(ctx, de);
}



output_context_t *
output_new(output_format_t fmt, const char *fname)
{
    switch (fmt) {
    case OUTPUT_FMT_CM:
	return output_cm_new(fname);
    case OUTPUT_FMT_DB:
	return output_db_new(fname);
    default:
	return NULL;
    }
}

