#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "types.h"
#include "xmalloc.h"
#include "romutil.h"
#include "util.h"
#include "dbh.h"
#include "funcs.h"
#include "r.h"
#include "parse.h"

static DB *db;
static struct game *g;
static char *prog_name, *prog_version;
/* XXX: every game is only allowed 1000 roms */
static struct rom r[1000], s[1000];
static struct disk d[10];
static int nr, ns, nd;
static enum { OUTSIDE, IN_ROM, IN_DISK, IN_SAMPLE } state;
static int romhashtypes, diskhashtypes;
static int nlost;
static char **lostchildren;
static int *lostchildren_to_do;
static int lostmax;

static int ngames, sgames;
static char **games;

static int nextra, sextra;
static char **extra;

static int nsl, ssl;
static char **sl;

char *parse_errstr;

#define CHECK_IN_GAME						\
	if (g == NULL) {					\
	    parse_errstr = "parse error: no game started";	\
	    return -1;						\
	}

#define CHECK_STATE(s)						\
	CHECK_IN_GAME;						\
	if (state != s) {					\
	    parse_errstr = "parse error: in wrong state";	\
	    return -1;						\
	}



static int add_extra_list(const char *);
static int add_name_list(const char *);
static int add_sample_list(const char *);
static void familymeeting(DB *, struct game *, struct game *);
static int game_add(DB *, struct game *);
static int lost(struct game *);



int
parse(DB *mydb, const char *fname)
{
    FILE *fin;
    int stillost, c;
    int i;
    struct game *parent;

    sgames = ngames = 0;

    db = mydb;

    if (fname == NULL) {
	fin = stdin;
	seterrinfo("*stdin*", NULL);
    }
    else {
	if ((fin=fopen(fname, "r")) == NULL) {
	    myerror(ERRSTR, "can\'t open romlist file `%s'", fname);
	    return -1;
	}
	seterrinfo(fname, NULL);
    }
    
    lostmax = 100;
    lostchildren = (char **)xmalloc(lostmax*sizeof(char *));
    lostchildren_to_do = (int *)xmalloc(lostmax*sizeof(int));

    romhashtypes = diskhashtypes = 0;
    nlost = 0;
    g = NULL;
    prog_name = prog_version = NULL;

    c = getc(fin);
    ungetc(c, fin);

    if (c == '<')
	parse_xml(fin);
    else
	parse_cm(fin);

    fclose(fin);

    if (nlost > 0)
	stillost = 1;
    while (stillost > 0) {
	stillost = 0;
	for (i=0; i<nlost; i++) {
	    if (lostchildren[i]==NULL) {
		/* this child is already done */
		continue;
	    }
	    /* get current lost child from database, get parent,
	       look if parent is still lost, if not, do child */
	    if ((g=r_game(db, lostchildren[i]))==NULL) {
		myerror(ERRDEF, "internal database error: "
			"child not in database");
		return 1;
	    }
	    if (lostchildren_to_do[i] & 1) {
		if ((parent=r_game(db, g->cloneof[0]))==NULL) {
		    myerror(ERRDEF, "input database not consistent: "
			    "parent %s not found", g->cloneof[0]);
		    return 1;
		}
		if (lost(parent)) {
		    stillost = 1;
		    game_free(parent, 1);
		} else {
		    /* parent found */
		    familymeeting(db, parent, g);
		    w_game(db, parent);
		    game_free(parent, 1);
		    w_game(db, g);
		    if (lostchildren_to_do[i] == 1) {
			free(lostchildren[i]);
			lostchildren[i] = NULL;
			game_free(g, 1);
		    }
		    lostchildren_to_do[i] &= ~1;
		}
	    }
	    if (lostchildren_to_do[i] & 2) {
		/* swap sample info with rom info */
		game_swap_rs(g);
		if ((parent=r_game(db, g->cloneof[0]))==NULL) {
		    myerror(ERRDEF, "input database not consistent: "
			    "parent %s not found", g->cloneof[0]);
		    return 1;
		}
		game_swap_rs(parent);
		if (lost(parent)) {
		    stillost = 1;
		    game_free(parent, 1);
		} else {
		    /* parent found */
		    familymeeting(db, parent, g);
		    /* swap back parent */
		    game_swap_rs(parent);
		    w_game(db, parent);
		    game_free(parent, 1);
		    /* swap back child */
		    game_swap_rs(g);
		    w_game(db, g);
		    if (lostchildren_to_do[i] == 2) {
			free(lostchildren[i]);
			lostchildren[i] = NULL;
		    }
		    lostchildren_to_do[i] &= ~2;
		}
		game_free(g, 1);
	    }
	}
    }

    /* write list of all known names to db */
    qsort((char **)games, ngames, sizeof(char *),
	  (int (*)(const void *, const void *))strpcasecmp);
    w_list(db, "/list", games, ngames);

    /* write list of all games with samples to db */
    qsort(sl, nsl, sizeof(char *),
	  (int (*)(const void *, const void *))strpcasecmp);
    w_list(db, "/sample_list", sl, nsl);

    /* write list of all extra files to db */
    qsort(extra, nextra, sizeof(char *),
	  (int (*)(const void *, const void *))strpcasecmp);
    w_list(db, "/extra_list", extra, nextra);

    w_hashtypes(db, romhashtypes, diskhashtypes);
    
    free(lostchildren);

    w_prog(db, prog_name, prog_version);
    free(prog_name);
    free(prog_version);

    return 0;
}



int
parse_disk_end(void)
{
    CHECK_STATE(IN_DISK);
    
    nd++;

    state = OUTSIDE;

    return 0;
}



int
parse_disk_md5(const char *attr)
{
    CHECK_STATE(IN_DISK);

    if (hex2bin(d[nd].hashes.md5, attr,
		sizeof(d[nd].hashes.md5)) != 0) {
	parse_errstr = "invalid argument for md5";
	return -1;
    }
    d[nd].hashes.types |= GOT_MD5;
    diskhashtypes |= GOT_MD5;

    return 0;
}




int
parse_disk_name(const char *attr)
{
    CHECK_STATE(IN_DISK);
    
    d[nd].name = xstrdup(attr);
    /* add to list of extra files */
    add_extra_list(d[nd].name);

    return 0;
}



int
parse_disk_sha1(const char *attr)
{
    CHECK_STATE(IN_DISK);

    if (hex2bin(d[nd].hashes.sha1, attr,
		sizeof(d[nd].hashes.sha1)) != 0) {
	parse_errstr = "invalid argument for sha1";
	return -1;
    }
    d[nd].hashes.types |= GOT_SHA1;
    diskhashtypes |= GOT_SHA1;

    return 0;
}



int
parse_disk_start()
{
    CHECK_STATE(OUTSIDE);

    hashes_init(&d[nd].hashes);

    state = IN_DISK;

    return 0;
}



int
parse_game_cloneof(const char *attr)
{
    CHECK_IN_GAME;
    
    g->cloneof[0] = xstrdup(attr);

    return 0;
}



int
parse_game_description(const char *attr)
{
    CHECK_IN_GAME;
    
    g->description = xstrdup(attr);

    return 0;
}



int
parse_game_end(void)
{
    int to_do;
    struct game *parent;

    CHECK_STATE(OUTSIDE);

    g->nrom = nr;
    g->rom = r;
    g->nsample = ns;
    g->sample = s;
    g->ndisk = nd;
    g->disk = d;

    /* add to list of games with samples */
    if (g->nsample > 0)
	add_sample_list(g->name);
    
    if (g->cloneof[0])
	if (strcmp(g->cloneof[0], g->name) == 0) {
	    free(g->cloneof[0]);
	    g->cloneof[0] = NULL;
	}
    
    to_do = 0;
    if (g->cloneof[0]) {
	if (((parent=r_game(db, g->cloneof[0]))==NULL) || 
	    lost(parent)) {
	    to_do = 1;
	    if (parent)
		game_free(parent, 1);
	}
	else {
	    familymeeting(db, parent, g);
	    w_game(db, parent);
	    game_free(parent, 1);
	}
    }
    
    if (g->sampleof[0])
	if (strcmp(g->sampleof[0], g->name) == 0) {
	    free(g->sampleof[0]);
	    g->sampleof[0] = NULL;
	}
    
    if (g->sampleof[0]) {
	if (((parent=r_game(db, g->sampleof[0]))==NULL) || 
	    lost(parent)) {
	    to_do += 2;
	    if (parent)
		game_free(parent, 1);
	}
	else {
	    game_swap_rs(g);
	    game_swap_rs(parent);
	    familymeeting(db, parent, g);
	    game_swap_rs(g);
	    game_swap_rs(parent);
	    w_game(db, parent);
	    game_free(parent, 1);
	}
    }
    
    if (to_do) {
	if (nlost > lostmax - 2) {
	    lostmax += 100;
	    lostchildren = (char **)
		xrealloc(lostchildren, lostmax*sizeof(char *));
	    lostchildren_to_do = (int *)
		xrealloc(lostchildren_to_do, lostmax*sizeof(int));
	}
	lostchildren[nlost] = xstrdup(g->name);
	lostchildren_to_do[nlost++] = to_do;
    }
    
    game_add(db, g);
    game_free(g, 0);

    g = NULL;

    return 0;
}




int
parse_game_name(const char *attr)
{
    CHECK_IN_GAME;

    g->name = xstrdup(attr);

    return 0;
}



int
parse_game_sampleof(const char *attr)
{
    CHECK_IN_GAME;

    g->sampleof[0] = xstrdup(attr);

    return 0;
}



int
parse_game_start(void)
{
    if (g) {
	parse_errstr = "game already started";
	return -1;
    }

    g = (struct game *)xmalloc(sizeof(struct game));
    g->name = g->description = g->cloneof[0] = g->cloneof[1]
	= g->sampleof[0] = g->sampleof[1] = NULL;
    g->nrom = g->nsample = 0;
    g->nclone = g->nsclone = 0;
    g->ndisk = 0;
    nr = ns = nd = 0;
    state = OUTSIDE;

    return 0;
}



int
parse_prog_name(const char *attr)
{
    prog_name = xstrdup(attr);

    return 0;
}



int
parse_prog_version(const char *attr)
{
    prog_version = xstrdup(attr);

    return 0;
}



int
parse_rom_crc(const char *attr)
{
    CHECK_STATE(IN_ROM);

    r[nr].hashes.crc = strtoul(attr, NULL, 16);
    r[nr].hashes.types |= GOT_CRC;
    romhashtypes |= GOT_CRC;

    return 0;
}



int
parse_rom_end(void)
{
    int deleted;
    int j;

    CHECK_STATE(IN_ROM);

    /* CRC == 0 was old way of indication no-good-dumps */
    if ((r[nr].hashes.types & GOT_CRC) && r[nr].hashes.crc == 0) {
	r[nr].hashes.types &= ~GOT_CRC;
	r[nr].flags = FLAGS_NODUMP;
    }
    
    /* omit duplicates */
    deleted = 0;
    for (j=0; j<nr; j++) {
	if (romcmp(r+j, r+nr, 0) == ROM_OK) {
	    deleted = 1;
	    break;
	}
    }
    if (!deleted) {
	for (j=0; j<nr; j++) {
	    if ((romcmp(r+j, r+nr, 0) == ROM_NAMERR
		 && r[j].merge && r[nr].merge
		 && !strcmp(r[j].merge, r[nr].merge))) {
		rom_add_name(r+j, r[nr].name);
		deleted = 1;
		break;
	    }
	}
    }
    if (deleted) {
	free(r[nr].merge);
	free(r[nr].name);
    }
    else
	nr++;

    state = OUTSIDE;

    return 0;
}



int
parse_rom_flags(const char *attr)
{
    CHECK_STATE(IN_ROM);

    if (strcmp(attr, "baddump") == 0)
	r[nr].flags = FLAGS_BADDUMP;
    else if (strcmp(attr, "nodump") == 0)
	r[nr].flags = FLAGS_NODUMP;

    return 0;
}



int
parse_rom_md5(const char *attr)
{
    CHECK_STATE(IN_ROM);

    if (hex2bin(r[nr].hashes.md5,
		attr, sizeof(r[nr].hashes.md5)) != 0) {
	parse_errstr = "invalid argument for md5";
	return -1;
    }
    r[nr].hashes.types |= GOT_MD5;
    romhashtypes |= GOT_MD5;

    return 0;
}




int
parse_rom_merge(const char *attr)
{
    CHECK_STATE(IN_ROM);

    r[nr].merge = xstrdup(attr);

    return 0;
}



int
parse_rom_name(const char *attr)
{
    CHECK_STATE(IN_ROM);

    r[nr].name = xstrdup(attr);

    return 0;
}



int
parse_rom_sha1(const char *attr)
{
    CHECK_STATE(IN_ROM);
    
    if (hex2bin(r[nr].hashes.sha1,
		attr, sizeof(r[nr].hashes.sha1)) != 0) {
	parse_errstr = "invalid argument for sha1";
	return -1;
    }
    r[nr].hashes.types |= GOT_SHA1;
    romhashtypes |= GOT_SHA1;

    return 0;
}



int
parse_rom_size(const char *attr)
{
    CHECK_STATE(IN_ROM);

    r[nr].size = strtol(attr, NULL, 10);

    return 0;
}



int
parse_rom_start(void)
{
    CHECK_STATE(OUTSIDE);

    r[nr].size = 0;
    hashes_init(&r[nr].hashes);
    r[nr].merge = NULL;
    r[nr].where = ROM_INZIP;
    r[nr].naltname = 0;
    r[nr].altname = NULL;
    r[nr].flags = FLAGS_OK;

    state = IN_ROM;

    return 0;
}



int
parse_sample_end(void)
{
    CHECK_STATE(IN_SAMPLE);
    
    ns++;

    state = OUTSIDE;

    return 0;
}



int
parse_sample_name(const char *attr)
{
    CHECK_STATE(IN_SAMPLE);

    s[ns].name = xstrdup(attr);

    return 0;
}




int
parse_sample_start(void)
{
    CHECK_STATE(OUTSIDE);

    s[ns].name = NULL;
    s[ns].merge = NULL;
    s[ns].altname = NULL;
    s[ns].naltname = s[ns].size = 0;
    hashes_init(&s[ns].hashes);
    s[ns].where = 0;

    state = IN_SAMPLE;

    return 0;
}



static int
add_extra_list(const char *s)
{
    if (nextra >= sextra) {
	sextra += 1024;
	if (nextra == 0)
	    extra = (char **)xmalloc(sizeof(char *)*sextra);
	else
	    extra = (char **)xrealloc(extra, sizeof(char *)*sextra);
    }

    extra[nextra++] = xstrdup(s);

    return 0;
}



static int
add_name_list(const char *s)
{
    if (ngames >= sgames) {
	sgames += 1024;
	if (ngames == 0)
	    games = (char **)xmalloc(sizeof(char *)*sgames);
	else
	    games = (char **)xrealloc(games, sizeof(char *)*sgames);
    }

    games[ngames++] = xstrdup(s);

    return 0;
}

static int
add_sample_list(const char *s)
{
    if (nsl >= ssl) {
	ssl += 1024;
	if (nsl == 0)
	    sl = (char **)xmalloc(sizeof(char *)*ssl);
	else
	    sl = (char **)xrealloc(sl, sizeof(char *)*ssl);
    }

    sl[nsl++] = xstrdup(s);

    return 0;
}



static void
familymeeting(DB *db, struct game *parent, struct game *child)
{
    struct game *gparent;
    int i, j;
    
    /* tell grandparent of his new grandchild */
    if (parent->cloneof[0]) {
	gparent = r_game(db, parent->cloneof[0]);
	gparent->clone = (char **)xrealloc(gparent->clone,
					   (gparent->nclone+1)*sizeof(char *));
	gparent->clone[gparent->nclone++] = xstrdup(child->name);
	w_game(db, gparent);
	game_free(gparent, 0);
    }

    /* tell child of his grandfather */
    if (parent->cloneof[0])
	child->cloneof[1] = xstrdup(parent->cloneof[0]);

    /* tell father of his child */
    parent->clone = (char **)xrealloc(parent->clone,
				      sizeof (char *)*(parent->nclone+1));
    parent->clone[parent->nclone++] = xstrdup(child->name);

    /* look for roms in parent */
    for (i=0; i<child->nrom; i++)
	for (j=0; j<parent->nrom; j++)
	    if (romcmp(parent->rom+j, child->rom+i, 1)==ROM_OK) {
		child->rom[i].where = parent->rom[j].where + 1;
		break;
	    }

    return;
}



static int
game_add(DB* db, struct game *g)
{
    int err;
    
    err = w_game(db, g);

    if (err != 0) {
	myerror(ERRSTR, "can't write game `%s' to db", g->name);
    }
    else
	add_name_list(g->name);

    return err;
}



static int
lost(struct game *a)
{
    int i;

    if (a->cloneof[0] == NULL)
	return 0;

    for (i=0; i<a->nrom; i++)
	if (a->rom[i].where != ROM_INZIP)
	    return 0;

    return 1;
}
