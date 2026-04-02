#include "mapstate.h"
#include "ncurses_screen.h"

int main() {
    const int FRAMES_PER_SECOND = 120;
    NcursesScreen screen(FRAMES_PER_SECOND);
    MapState map_state(30, 60, 1, screen);
    map_state.run_level();

    return 0;
}
