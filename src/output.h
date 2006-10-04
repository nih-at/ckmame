#ifndef HAD_OUTPUT_H
#define HAD_OUTPUT_H

/*
  $NiH: output.h,v 1.3 2006/05/26 21:46:50 dillo Exp $

  output.h -- output game info
  Copyright (C) 2006 Dieter Baron and Thomas Klausner

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



#include "dat.h"
#include "game.h"



typedef struct output_context output_context_t;

struct output_context {
    int (*close)(output_context_t *);
    int (*output_game)(output_context_t *, game_t *);
    int (*output_header)(output_context_t *, dat_entry_t *);
};

enum output_format {
    OUTPUT_FMT_CM,
    OUTPUT_FMT_DB
};

typedef enum output_format output_format_t;



output_context_t *output_cm_new(const char *);
output_context_t *output_db_new(const char *);

int output_close(output_context_t *);
int output_game(output_context_t *, game_t *);
int output_header(output_context_t *, dat_entry_t *);
output_context_t *output_new(output_format_t, const char *);

#endif /* output.h */
