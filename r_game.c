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

    if (ddb_lookup(db, &k, &v) != 0) {
	free(k.data);
	return NULL;
    }
    data = v.data;

    game = (struct game *)xmalloc(sizeof(struct game));
    
    game->name = xstrdup(name);
    game->description = r__string(&v);
    game->cloneof[0] = r__string(&v);
    game->cloneof[1] = r__string(&v);
    game->nclone = r__array(&v, r__pstring, (void *)&game->clone,
			    sizeof(char *));
    game->nrom = r__array(&v, r__rom, (void *)&game->rom, sizeof(struct rom));
    game->sampleof[0] = r__string(&v);
    game->sampleof[1] = r__string(&v);
    game->nsclone = r__array(&v, r__pstring, (void *)&game->sclone,
			    sizeof(char *));
    game->nsample = r__array(&v, r__rom, (void *)&game->sample,
			     sizeof(struct rom));

    free(k.data);
    free(data);

    return game;
}
