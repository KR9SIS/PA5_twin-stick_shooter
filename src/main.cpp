#include <chrono>
#include <ncurses.h>
#include <thread>

enum ClrPrs { PLAYER_PAIR = 1, ENEMY_PAIR = 2 };

int main() {
    ::initscr(); // Starts ncurses.
    ::cbreak();  // Disables line buffering, making input available immediately.
    ::noecho();  // Prevents key presses from being printed to the screen.
    ::keypad(::stdscr,
             TRUE); // Enables reading of function keys (like arrow keys).
    ::curs_set(0);  // Hides the cursor.
    ::nodelay(::stdscr, TRUE); // Makes getch() non-blocking for concurrency.

    // Player coordinates.
    int player_y = 10;
    int player_x = 20;
    // Enemy coordinates.
    int enemy_y = 5;
    int enemy_x = 0;
    const int MAX_ENEMY_DIST = 200;    // Temporary, for the loop.
    const int FRAMES_PER_SECOND = 120; // E.g., 30, 60, 120, ...
    const int REFRESH_INTERVAL_MS = 1000 / FRAMES_PER_SECOND;

    bool is_running = true;

    while (is_running) {
        ::start_color();
        ::init_pair(ClrPrs::PLAYER_PAIR, COLOR_CYAN, COLOR_BLACK);
        ::init_pair(ClrPrs::ENEMY_PAIR, COLOR_RED, COLOR_BLACK);
        ::clear(); // Clears the screen buffer.
        ::mvprintw(0, 0, "Use w,a,s,d to move. Press q to quit.");

        attron(COLOR_PAIR(ClrPrs::PLAYER_PAIR));
        ::mvprintw(player_y, player_x,
                   "@"); // Move cursor to (player_y, player_x) and print there.
        attroff(COLOR_PAIR(ClrPrs::PLAYER_PAIR));

        attron(COLOR_PAIR(ClrPrs::ENEMY_PAIR));
        ::mvprintw(enemy_y, enemy_x,
                   "#"); // Move cursor to (enemy_y, enemy_x) and print there.
        attroff(COLOR_PAIR(ClrPrs::ENEMY_PAIR));

        ::refresh(); // Actually pushes the drawing to the screen.

        switch (int keyPressed = getch(); keyPressed) {
        case 'w':
            --player_y;
            break;
        case 's':
            ++player_y;
            break;
        case 'a':
            --player_x;
            break;
        case 'd':
            ++player_x;
            break;
        case 'q':
            is_running = false;
            break;
        default:
            break;
        }

        // Makes the enemy go forward in a loop until it reaches MAX_ENEMY_DIST.
        enemy_x = (enemy_x + 1) % MAX_ENEMY_DIST;

        // Make this thread sleep for REFRESH_INTERVAL_MS between each loop
        // iteration. This basically lets us set the time between ticks.
        std::this_thread::sleep_for(
            std::chrono::milliseconds(REFRESH_INTERVAL_MS));
    }

    ::endwin(); // Ends ncurses mode and restores the terminal.
    return 0;
}