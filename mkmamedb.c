#include "types.h"
#include "dbl.h"

char *prg;

int
main(int argc, char **argv)
{
    DB *db;

    unlink("mame.db");

    db = db_open("mame", 1, 1);

    dbread_init();
    dbread(db, "db.txt");

    db_close(db);
}
