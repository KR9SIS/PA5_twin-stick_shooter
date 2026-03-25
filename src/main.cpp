#include "ncurses_screen.hpp"

namespace {
void run_game_loop(NcursesScreen const& screen) {
    // Player coordinates.
    int player_y = 10;
    int player_x = 20;
    // Enemy coordinates.
    int enemy_y = 5;
    int enemy_x = 0;
    const int MAX_ENEMY_DIST = 200;

    bool is_running = true;

    while (is_running) {
        screen.render_frame(player_y, player_x, enemy_y, enemy_x);
        screen.handle_input(player_y, player_x, is_running);

        // Makes the enemy go forward in a loop until it reaches MAX_ENEMY_DIST.
        enemy_x = (enemy_x + 1) % MAX_ENEMY_DIST;

        screen.sleep_until_next_frame();
    }
}
} // namespace

int main() {
    const int FRAMES_PER_SECOND = 120;
    NcursesScreen screen(FRAMES_PER_SECOND);
    run_game_loop(screen);

    return 0;
}