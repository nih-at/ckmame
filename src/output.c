#include "output.h"

int
output_close(output_context_t *ctx)
{
    return ctx->close(ctx);
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
