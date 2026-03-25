#include "mapstate.h"

MapState::MapState(size_t r, size_t c)
    : ROWS(r), COLUMNS(c), map(r, std::vector<std::unique_ptr<Entity>>(c)) {}

// take difficulty as const in map state
// instantiate enemies based on difficulty
/*
enemy-loop
while (game is running) {
    for e:enemies {
        e.act()
    }
this:thread sleep ms
}
*/

void MapState::run_level(int difficulty) {
    // while (game is running) { for (auto &e : enemies) e.act(); /* sleep */ }
}
