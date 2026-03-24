#include <chrono>
#include <ncurses.h>
#include <thread>

enum MyColorPairs { PLAYER_PAIR = 1, ENEMY_PAIR = 2 };

int main() {
    using enum MyColorPairs;

    ::initscr(); // Starts ncurses.
    ::cbreak();  // Disables line buffering, making input available immediately.
    ::noecho();  // Prevents key presses from being printed to the screen.
    ::keypad(::stdscr,
             TRUE); // Enables reading of function keys (like arrow keys).
    ::curs_set(0);  // Hides the cursor.
    ::nodelay(::stdscr, TRUE); // Makes getch() non-blocking for concurrency.

    // Player coordinates.
    int p_y = 10;
    int p_x = 20;
    // Enemy coordinates.
    int e_y = 5;
    int e_x = 0;
    const int MAX_ENEMY_DIST = 200;    // Temporary, for the loop.
    const int FRAMES_PER_SECOND = 120; // E.g., 30, 60, 120, ...
    const int REFRESH_INTERVAL_MS = 1000 / FRAMES_PER_SECOND;

    bool is_running = true;

    while (is_running) {
        ::start_color();
        ::init_pair(PLAYER_PAIR, COLOR_CYAN, COLOR_BLACK);
        ::init_pair(ENEMY_PAIR, COLOR_RED, COLOR_BLACK);
        ::clear(); // Clears the screen buffer.
        ::mvprintw(0, 0, "Use w,a,s,d to move. Press q to quit.");

        attron(COLOR_PAIR(PLAYER_PAIR));
        ::mvprintw(p_y, p_x, "@"); // Move cursor to (p_y, p_x) and print there.
        attroff(COLOR_PAIR(PLAYER_PAIR));

        attron(COLOR_PAIR(ENEMY_PAIR));
        ::mvprintw(e_y, e_x, "#"); // Move cursor to (e_y, e_x) and print there.
        attroff(COLOR_PAIR(ENEMY_PAIR));

        ::refresh(); // Actually pushes the drawing to the screen.

        switch (int keyPressed = getch(); keyPressed) {
        case 'w':
            --p_y;
            break;
        case 's':
            ++p_y;
            break;
        case 'a':
            --p_x;
            break;
        case 'd':
            ++p_x;
            break;
        case 'q':
            is_running = false;
            break;
        default:
            break;
        }

        // Makes the enemy go forward in a loop until it reaches MAX_ENEMY_DIST.
        e_x = (e_x + 1) % MAX_ENEMY_DIST;

        // Concurrency, babyyyy!
        std::this_thread::sleep_for(
            std::chrono::milliseconds(REFRESH_INTERVAL_MS));
    }

    ::endwin(); // Ends ncurses mode and restores the terminal.
    return 0;
}