#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <regex.h>

#include "error.h"
#include "types.h"

char *extract(char *s, regmatch_t m);

enum regs { r_game, r_name, r_cloneof, r_romof, r_rom, r_sampleof, r_sample,
	    r_gameend, r_END };

static char *sregs[] = {
    "[ \t]*game[ \t]*(",
    "[ \t]*name[ \t]*([^ ]*)",
    "[ \t]*cloneof[ \t]*([^ ]*)",
    "[ \t]*romof[ \t]*([^ ]*)",
    "[ \t]*romof[ \t]*\\([ \t]*name[ \t]*([^ ]*)[ \t]*size[ \t]*([^ ]*)[ \t]*crc[ \t]*([^ ]*)[ \t]*\\)",
    "[ \t]*sampleof[ \t]*([^ ]*)",
    "[ \t]*sample[ \t]*([^ ]*)",
    "[ \t]*\\)"
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
    
    return 0;
}



int
dbread(char *fname)
{
    FILE *fin;
    regmatch_t match[6];
    char l[8192], *p;
    int ingame, i;
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
	    ingame = 1;
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
	    r[nr].crc = strtol(p, NULL, 16);
	    free(p);
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
	    if (nr) {
		g->rom = (struct rom *)malloc(sizeof(struct rom)*nr);
		if (g->rom)
		    memcpy(g->rom, r, sizeof(struct rom)*nr);
	    }
	    if (ns) {
		g->sample = (struct rom *)malloc(sizeof(struct rom)*ns);
		if (g->sample)
		    memcpy(g->sample, s, sizeof(struct rom)*ns);
	    }
	    
	    game_add(g);
	}
    }

    return 0;
}
		


char *
extract(char *s, regmatch_t m)
{
    char *t;
    
    if ((t=(char *)malloc(m.rm_eo-m.rm_so+1)) == NULL) {
	myerror(ERRDEF, "malloc failure");
	exit(1);
    }
    
    strncpy(t, s+m.rm_so, m.rm_eo-m.rm_so);
    t[m.rm_eo-m.rm_so] = '\0';
    
    return t;
}
