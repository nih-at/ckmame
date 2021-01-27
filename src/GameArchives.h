#ifndef GAME_ARCHIVES_H
#define GAME_ARCHIVES_H

#include "archive.h"

class GameArchives {
public:
    ArchivePtr archive[TYPE_MAX];
    
    Archive *operator[](filetype_t ft) const { return archive[ft].get(); }
    Archive *operator[](size_t i) const { return archive[i].get(); }
};

#endif // GAME_ARCHIVES_H
