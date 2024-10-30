#include "Game.h"

bool Game::is_mia() const {
    for (size_t ft = 0; ft < TYPE_MAX; ft++) {
        if (std::any_of(files[ft].begin(), files[ft].end(), [](const Rom& rom) {return rom.mia;})) {
            return true;
        }
    }
    return false;
}
