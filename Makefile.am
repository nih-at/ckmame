AUTOMAKE_OPTIONS=dist-zip 

LDADD=@LIBOBJS@

bin_PROGRAMS=ckmame mkmamedb dumpgame
ckmame_SOURCES=ckmame.c error.c unzip.c zip-supp.c util.c marry.c \
		match.c malloc.c r_game.c r_list.c r_util.c dbl.c treetrav.c
mkmamedb_SOURCES=mkmamedb.c dbl.c dbread.c w_game.c w_list.c error.c malloc.c \
	w_util.c r_game.c r_util.c util.c
dumpgame_SOURCES=dump_game.c error.c malloc.c r_game.c r_util.c dbl.c util.c
noinst_HEADERS=unzip.h error.h types.h w.h funcs.h r.h dbl.h db-db.h mkmamedb.h


LIBS = -lz
