/* read struct game from db */

#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "dbl.h"
#include "xmalloc.h"
#include "r.h"



struct game *
r_game(DB *db, char *name)
{
    DBT k, v;
    struct game *game;
    void *data;

    k.size = strlen(name);
    k.data = xmalloc(k.size);
    strncpy(k.data, name, k.size);

    if (db_lookup(db, &k, &v) != 0) {
	free(k.data);
	return NULL;
    }
    data = v.data;

    game = (struct game *)xmalloc(sizeof(struct game));
    
    game->name = strdup(name);
    game->cloneof[0] = r__string(&v);
    game->cloneof[1] = r__string(&v);
    game->nclone = r__array(&v, r__pstring, &game->clone, sizeof(char *));
    game->nrom = r__array(&v, r__rom, &game->rom, sizeof(struct rom));
    game->sampleof = r__string(&v);
    game->nsample = r__array(&v, r__rom, &game->sample, sizeof(struct rom));

    free(k.data);
    free(data);

    return game;
}
