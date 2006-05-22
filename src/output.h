#ifndef HAD_OUTPUT_H
#define HAD_OUTPUT_H

#include "dat.h"
#include "game.h"



typedef struct output_context output_context_t;

struct output_context {
    int (*close)(output_context_t *);
    int (*output_game)(output_context_t *, game_t *);
    int (*output_header)(output_context_t *, dat_entry_t *);
};



output_context_t *output_dat_new(const char *);
output_context_t *output_db_new(const char *);

int output_close(output_context_t *);
int output_game(output_context_t *, game_t *);
int output_header(output_context_t *, dat_entry_t *);

#endif /* output.h */
