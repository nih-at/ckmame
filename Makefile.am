AUTOMAKE_OPTIONS=dist-zip 

bin_PROGRAMS=ckmame mkmamedb
ckmame_SOURCES=ckmame.c error.c unzip.c zip-supp.c util.c marry.c \
		match.c malloc.c r_game.c r_util.c dbl.c
mkmamedb_SOURCES=mkmamedb.c dbl.c dbread.c w_game.c w_list.c error.c malloc.c \
	w_util.c r_game.c r_util.c util.c
noinst_HEADERS=unzip.h error.h types.h w.h


LIBS = -lz
