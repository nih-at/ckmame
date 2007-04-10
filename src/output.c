/*
  $NiH: output.c,v 1.4 2007/04/10 16:26:46 dillo Exp $

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

