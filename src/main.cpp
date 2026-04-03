#include "mapstate.h"
#include "ncurses_screen.h"

int main() {
    const int FRAMES_PER_SECOND = 120;
    NcursesScreen screen(FRAMES_PER_SECOND);

    uint16_t difficulty = 20;
    uint16_t rows = 50;
    uint16_t columns = 100;

    uint8_t lvl = 0;
    bool won = true;
    while (lvl < 3 && won) {
        MapState map_state(rows, columns, difficulty, screen);
        won = map_state.run_level();
        difficulty *= 2;
        rows /= 2;
        columns /= 2;

        lvl++;
    }

    return 0;
}
