/*
  $NiH: dbread.c,v 1.33 2004/02/05 17:32:30 dillo Exp $

  dbread.c -- parsing listinfo output, creating mamedb
  Copyright (C) 1999, 2003 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "error.h"
#include "types.h"
#include "mkmamedb.h"
#include "xmalloc.h"
#include "romutil.h"
#include "util.h"
#include "dbh.h"
#include "funcs.h"
#include "r.h"

static int ngames, sgames;
static char **games;

static int nextra, sextra;
static char **extra;

static int nsl, ssl;
static char **sl;

static char *gettok(char **p);
static int add_name_list(char *s);
static int add_extra_list(char *s);
static int add_sample_list(char *s);
static int game_add(DB* db, struct game *g);
static void familymeeting(DB* db, struct game *parent, struct game *child);
static int lost(struct game *a);

void game_free(struct game *g, int fullp);

enum regs { r_gamestart, r_name, r_romof, r_rom, r_sampleof, r_sample,
	    r_gameend, r_END };



int
dbread_init(void)
{
    sgames = ngames = 0;

    return 0;
}



enum parse_state { st_top, st_game, st_prog };

int
dbread(DB* db, char *fname)
{
    FILE *fin;
    char b[8192], *cmd, *p, *l;
    char *prog_name, *prog_version;
    enum parse_state state;
    int i, j, deleted;
    /* XXX: every game is only allowed 1000 roms */
    struct rom r[1000], s[1000];
    struct disk d[10];
    struct game *g;
    struct game *parent;
    char **lostchildren;
    int *lostchildren_to_do;
    int nlost, lostmax, stillost;
    int nr, ns, nd, lineno;
    int to_do;

    if ((fin=fopen(fname, "r")) == NULL) {
	myerror(ERRSTR, "can\'t open romlist file `%s'", fname);
	return -1;
    }

    lostmax = 100;
    lostchildren = (char **)xmalloc(lostmax*sizeof(char *));
    lostchildren_to_do = (int *)xmalloc(lostmax*sizeof(int));
    prog_name = prog_version = NULL;

    seterrinfo(fname, NULL);
    
    nlost = nr = ns = nd = 0;
    lineno = 0;
    state = st_top;
    
    while (fgets(l=b, 8192, fin)) {
	lineno++;
	if (b[strlen(b)-1] != '\n') {
	    cmd = gettok(&l);
	    if ((cmd == NULL) || (strcmp(cmd, "history"))) {
		myerror(ERRFILE, "%d: warning: line too long (ignored)",
			lineno);
	    }
	    while (fgets(b, 8192, fin)) {
		if (b[strlen(b)-1] == '\n')
		    break;
	    }
	    continue;
	}
		
	    
	cmd = gettok(&l);
	if (cmd == NULL)
	    continue;

	switch (state) {
	case st_top:
	    /* game/resource for MAME/Raine, machine for MESS */
	    if (strcmp(cmd, "game") == 0 || strcmp(cmd, "machine") == 0
		||strcmp(cmd, "resource") == 0) {
		g = (struct game *)xmalloc(sizeof(struct game));
		g->name = g->description = g->cloneof[0] = g->cloneof[1]
		    = g->sampleof[0] = g->sampleof[1] = NULL;
		g->nrom = g->nsample = 0;
		g->nclone = g->nsclone = 0;
		g->ndisk = 0;
		nr = ns = nd = 0;
		state = st_game;
	    }
	    else if (strcmp(cmd, "emulator") == 0)
		state = st_prog;
	    break;
	    
	case st_game:
	    if (strcmp(cmd, "name") == 0) {
		g->name = xstrdup(gettok(&l));
	    }
	    else if (strcmp(cmd, "description") == 0) {
		g->description = xstrdup(gettok(&l));
	    }
	    else if (strcmp(cmd, "romof") == 0) {
		g->cloneof[0] = xstrdup(gettok(&l));
	    }
	    else if (strcmp(cmd, "rom") == 0) {
		gettok(&l);
		if (strcmp(gettok(&l), "name") != 0) {
		    /* XXX: error */
		    myerror(ERRFILE, "%d: expected token (name) not found",
			    lineno);
		    break;
		}
		r[nr].name = xstrdup(gettok(&l));
		r[nr].size = 0;
		r[nr].crctypes = 0;
		r[nr].crc = 0;
		memset(r[nr].sha1, 0, sizeof(r[nr].sha1));
		r[nr].merge = NULL;
		r[nr].where = ROM_INZIP;
		r[nr].naltname = 0;
		r[nr].altname = NULL;
		r[nr].flags = FLAGS_OK;

		/* read remaining tokens and look for known tokens */
		while ((p=gettok(&l)) != NULL) {
		    if (strcmp(p, "crc") == 0 || strcmp(p, "crc32") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token crc missing argument",
				    lineno);
			    break;
			}
			r[nr].crc = strtoul(p, NULL, 16);
			r[nr].crctypes = GOT_CRC;
		    } else if (strcmp(p, "flags") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token flags missing argument",
				    lineno);
			    break;
			}
			if (strcmp(p, "baddump") == 0)
			    r[nr].flags = FLAGS_BADDUMP;
			else if (strcmp(p, "nodump") == 0)
			    r[nr].flags = FLAGS_NODUMP;
		    } else if (strcmp(p, "merge") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token merge missing argument",
				    lineno);
			    break;
			}
			r[nr].merge = xstrdup(p);
		    }
		    else if (strcmp(p, "sha1") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token sha1 missing argument",
				    lineno);
			    break;
			}
			
			if (hex2bin(r[nr].sha1, p, sizeof(r[nr].sha1)) != 0) {
			    myerror(ERRFILE, "%d: token sha1 argument invalid",
				    lineno);
			    break;
			}
			r[nr].crctypes = GOT_SHA1;
		    }
		    else if (strcmp(p, "size") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token size missing argument",
				    lineno);
			    break;
			}
			r[nr].size = strtol(p, NULL, 10);
		    }
		    /*
		      else
		      myerror(ERRFILE, "%d: ignoring token `%s'", lineno, p);
		    */
		}

		/* CRC == 0 was old way of indication no-good-dumps */
		if (r[nr].crc == 0)
		    r[nr].flags = FLAGS_NODUMP;

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
	    }
	    else if (strcmp(cmd, "disk") == 0) {
		gettok(&l);
		if (strcmp(gettok(&l), "name") != 0) {
		    /* XXX: error */
		    myerror(ERRFILE, "%d: expected token (name) not found",
			    lineno);
		    break;
		}
		d[nd].name = xstrdup(gettok(&l));
		/* add to list of extra files */
		add_extra_list(d[nd].name);
		d[nd].crctypes = 0;
		memset(d[nd].sha1, 0, sizeof(d[nd].sha1));
		memset(d[nd].md5, 0, sizeof(d[nd].md5));

		/* read remaining tokens and look for known tokens */
		while ((p=gettok(&l)) != NULL) {
		    if (strcmp(p, "sha1") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token sha1 missing argument",
				    lineno);
			    break;
			}
			
			if (hex2bin(d[nd].sha1, p, sizeof(d[nd].sha1)) != 0) {
			    myerror(ERRFILE, "%d: token sha1 argument invalid",
				    lineno);
			    break;
			}
			d[nd].crctypes |= GOT_SHA1;
		    }
		    else if (strcmp(p, "md5") == 0) {
			if ((p=gettok(&l)) == NULL) {
			    /* XXX: error */
			    myerror(ERRFILE, "%d: token md5 missing argument",
				    lineno);
			    break;
			}
			
			if (hex2bin(d[nd].md5, p, sizeof(d[nd].md5)) != 0) {
			    myerror(ERRFILE, "%d: token md5 argument invalid",
				    lineno);
			    break;
			}
			d[nd].crctypes |= GOT_MD5;
		    }
		    /*
		      else
		      myerror(ERRFILE, "%d: ignoring token `%s'", lineno, p);
		    */
		}
		nd++;
	    }
	    else if (strcmp(cmd, "sampleof") == 0)
		g->sampleof[0] = xstrdup(gettok(&l));
	    else if (strcmp(cmd, "sample") == 0) {
		s[ns].name = xstrdup(gettok(&l));
		s[ns].merge = NULL;
		s[ns].altname = NULL;
		s[ns].naltname = s[ns].size = 0;
		s[ns].crc = s[ns].where = 0;
		ns++;
	    }
	    else if (strcmp(cmd, "archive") == 0) {
		/* XXX: archive names */
	    }
	    else if (strcmp(cmd, ")") == 0) {
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

		state = st_top;
	    }
	    break;
	    
	case st_prog:
	    if (strcmp(cmd, "name") == 0)
		prog_name = gettok(&l);
	    else if (strcmp(cmd, "version") == 0)
		prog_version = gettok(&l);
	    else if (strcmp(cmd, ")") == 0)
		state = st_top;
	    break;
	}
    }

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
    qsort(games, ngames, sizeof(char *),
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

    w_prog(db, prog_name, prog_version);

    free(lostchildren);
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
add_name_list(char *s)
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
add_sample_list(char *s)
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



static int
add_extra_list(char *s)
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



static char *
gettok(char **p)
{
    char *s, *e;

    s = *p;
    
    if (s == NULL)
	return NULL;

    s += strspn(s, " \t");

    switch (*s) {
    case '\0':
    case '\n':
    case '\r':
	*p = NULL;
	return NULL;
	
    case '\"':
	s++;
	e = s+strcspn(s, "\"");
	*e = 0;
	break;

    default:
	e = s+strcspn(s, " \t\n\r");
	*e = 0;
	break;
    }

    *p = e+1;
    return s;
}
