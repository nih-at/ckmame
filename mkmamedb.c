#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "dbl.h"
#include "funcs.h"
#include "mkmamedb.h"
#include "error.h"

char *prg;

int
main(int argc, char **argv)
{
    DB *db;
    char *dbname;
    
    prg = argv[0];
    dbname = db_name("mame");

    remove(dbname);
    db = db_open(dbname, 0, 1);

    if (db==NULL) {
	myerror(ERRSTR, "can't create db '%s': %s", dbname, db_error());
	exit(1);
    }

    dbread_init();
    dbread(db, "db.txt");

    db_close(db);

    free(dbname);
    return 0;
}
