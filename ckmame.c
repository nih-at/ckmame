#include "types.h"
#include "dbl.h"

char *prg;



int
main(int argc, char **argv)
{
    DB *db;
    
    prg = argv[0];

    db = db_open("mame", 1, 0);
    
    check_game(db, argv[1]);

    return 0;
}
