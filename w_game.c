/* write struct game to db */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "dbh.h"
#include "util.h"
#include "xmalloc.h"
#include "w.h"



int
w_game(DB *db, struct game *game)
{
    int err;
    DBT k, v;

    k.size = strlen(game->name);
    k.data = xmalloc(k.size);
    strncpy(k.data, game->name, k.size);

    v.data = NULL;
    v.size = 0;

    if (game->nclone) {
	qsort(game->clone, game->nclone, sizeof(char *),
	      (int (*)(const void *, const void *))strpcasecmp);
    }
    
    w__string(&v, game->cloneof[0]);
    w__string(&v, game->cloneof[1]);
    w__array(&v, w__pstring, game->clone, sizeof(char *), game->nclone);
    w__array(&v, w__rom, game->rom, sizeof(struct rom), game->nrom);
    w__string(&v, game->sampleof);
    w__array(&v, w__rom, game->sample, sizeof(struct rom), game->nsample);

    err = db_insert(db, &k, &v);

    free(k.data);
    free(v.data);

    return err;
}
