#include <stdio.h>
#include "types.h"
#include "dbl.h"
#include "funcs.h"
#include "mkmamedb.h"

char *prg;

int
main(int argc, char **argv)
{
    DB *db;

    prg = argv[0];
    remove("mame.db");

    db = db_open("mame", 1, 1);

    dbread_init();
    dbread(db, "db.txt");

    db_close(db);

    return 0;
}
