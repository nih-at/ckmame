#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>

#include "error.h"
#include "types.h"
#include "dbl.h"

int ngames, sgames;
char **games;

static char *extract(char *s, regmatch_t m);
static int game_add(DB* db, struct game *g);

void game_free(struct game *g);

enum regs { r_game, r_name, r_cloneof, r_romof, r_rom, r_sampleof, r_sample,
	    r_gameend, r_END };

static char *sregs[] = {
    "^[ \t]*game[ \t]*\\(",
    "^[ \t]*name[ \t]*([^ \t\n]*)",
    "^[ \t]*cloneof[ \t]*([^ \t\n]*)",
    "^[ \t]*romof[ \t]*([^ \t\n]*)",
    "^[ \t]*rom[ \t]+\\([ \t]+name[ \t]+([^ ]*)[ \t]+size[ \t]+([^ ]*)[ \t]+crc[ \t]+([^ ]*)[ \t]+\\)",
    "^[ \t]*sampleof[ \t]*([^ \t\n]*)",
    "^[ \t]*sample[ \t]*([^ \t\n]*)",
    "^[ \t]*\\)",
};

regex_t regs[r_END];



int
dbread_init(void)
{
    int i, err;
    char b[8192];

    for (i=0; i<r_END; i++) {
        if ((err=regcomp(&regs[i], sregs[i], REG_EXTENDED|REG_ICASE)) != 0) {
	    regerror(err, &regs[i], b, 8192);
	    myerror(ERRDEF, "can't compile regex pattern %d: %s", i, b);
	    return -1;
	}
    }

    sgames = ngames = 0;

    return 0;
}



int
dbread(DB* db, char *fname)
{
    FILE *fin;
    regmatch_t match[6];
    char l[8192], *p;
    int ingame, i;
    /* XXX: Boese, sehr, sehr, boese. Es gibt nur 100 roms? Ha, ha. */
    struct rom r[100], s[100];
    struct game *g;
    int nr, ns;

    if ((fin=fopen(fname, "r")) == NULL) {
	myerror(ERRDEF, "can\'t open romlist file `%s': %s",
		fname, strerror(errno));
	return -1;
    }

    nr = ns = ingame = 0;
    while (fgets(l, 8192, fin)) {
	for (i=0; i<r_END; i++)
	    if (regexec(&regs[i], l, 6, match, 0) == 0)
		break;

	switch (i) {
	case r_game:
	    g = (struct game *)xmalloc(sizeof(struct game));
	    g->name = g->cloneof = g->romof = g->sampleof = NULL;
	    g->nrom = g->nsample = 0;
	    ingame = 1;
	    nr = ns = 0;
	    break;

	case r_name:
	    g->name = extract(l, match[1]);
	    break;

	case r_cloneof:
	    g->cloneof = extract(l, match[1]);
	    break;

	case r_romof:
	    g->romof = extract(l, match[1]);
	    if (g->cloneof && strcmp(g->cloneof, g->romof) == 0) {
		free(g->romof);
		g->romof = NULL;
	    }
	    break;

	case r_rom:
	    r[nr].name = extract(l, match[1]);
	    p = extract(l, match[2]);
	    r[nr].size = strtol(p, NULL, 10);
	    free(p);
	    p = extract(l, match[3]);
	    r[nr].crc = strtoul(p, NULL, 16);
	    free(p);
	    r[nr].where = ROM_INZIP;
	    nr++;
	    break;

	case r_sampleof:
	    g->sampleof = extract(l, match[1]);
	    break;

	case r_sample:
	    s[ns].name = extract(l, match[1]);
	    p = extract(l, match[2]);
	    s[ns].size = strtol(p, NULL, 10);
	    free(p);
	    ns++;
	    break;

	case r_gameend:
	    g->nrom = nr;
	    g->rom = r;
	    g->nsample = ns;
	    g->sample = s;
	    	    
	    game_add(db, g);
	    game_free(g);
	    break;
	}
    }

    return 0;
}
		


static char *
extract(char *s, regmatch_t m)
{
    char *t;
    
    t=(char *)xmalloc(m.rm_eo-m.rm_so+1);
    
    strncpy(t, s+m.rm_so, m.rm_eo-m.rm_so);
    t[m.rm_eo-m.rm_so] = '\0';
    
    return t;
}



int
game_add(DB* db, struct game *g)
{
    int err;
    
    /* XXX: check for roms in cloneof/romof */
    /* XXX: add g to clones/roms list of parent */
    
    err = w_game(db, g);

    if (err != 0) {
	myerror(ERRDEF, "can't write game `%s' to db: %s",
		g->name, strerror(errno));
    }
    else
	add_name(g->name);

    return err;
}



static int
add_name(char *s)
{
    if (ngames >= sgames) {
	sgames += 1024;
	if (ngames == 0)
	    games = (char **)xmalloc(sizeof(char *)*sgames);
	else
	    games = (char **)xrealloc(games, sizeof(char *)*sgames);
    }

    games[ngames++] = strdup(s);
}



void
game_free(struct game *g)
{
    int i;

    free(g->name);
    free(g->cloneof);
    free(g->romof);
    free(g->sampleof);
    for (i=0; i<g->nrom; i++)
	free(g->rom[i].name);
    /* XXX: dbread does'nt allocate this: free(g->rom); */
    for (i=0; i<g->nsample; i++)
	free(g->sample[i].name);
    /* XXX: dbread does'nt allocate this: free(g->rom); */

    free(g);
}
