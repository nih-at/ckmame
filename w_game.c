/* write struct game to db */

#include <stddef.h>

#include "types.h"
#include "dbl.h"
#include "w.h"



int
w_game(DB *db, struct game *game)
{
    int i, len, err;
    DBT k, v;

    k.size = strlen(game->name);
    k.data = xmalloc(k.size);
    strncpy(k.data, game->name, k.size);

    v.data = NULL;
    v.size = 0;

    w__string(&v, game->cloneof[0]);
    w__string(&v, game->cloneof[1]);
    w__array(&v, w__string, game->clone, sizeof(char *), game->nclone);
    w__array(&v, w__rom, game->rom, sizeof(struct rom), game->nrom);
    w__string(&v, game->sampleof);
    w__array(&v, w__rom, game->sample, sizeof(struct rom), game->nsample);

    err = db_insert(db, &k, &v);

    free(k.data);
    free(v.data);

    return err;
}
