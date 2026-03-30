#include "ncurses_screen.h"

#include <chrono>
#include <memory>
#include <ncurses.h>
#include <thread>
#include <vector>

namespace {
enum ClrPr { PLAYER_PAIR = 1, ENEMY_PAIR = 2 };
}

NcursesScreen::NcursesScreen(int frames_per_second)
    : refresh_interval_ms_(1000 / frames_per_second) {
    ::initscr(); // Init ncurses library and set up the screen for drawing.
    ::cbreak();  // Disable line buffering, so input is sent immediately.
    ::noecho();  // Don't print user input to the screen.
    ::keypad(::stdscr, TRUE);  // Enable special keys, e.g. arrow keys.
    ::curs_set(0);             // Hide the cursor.
    ::nodelay(::stdscr, TRUE); // Don't block on user input.

    ::start_color(); // Enable color functionality in ncurses.
    ::init_pair(ClrPr::PLAYER_PAIR, COLOR_CYAN, COLOR_BLACK);
    ::init_pair(ClrPr::ENEMY_PAIR, COLOR_RED, COLOR_BLACK);
}

NcursesScreen::~NcursesScreen() {
    ::endwin(); // Restore the terminal to its original state after exiting.
}

void NcursesScreen::render_frame(
    const std::shared_ptr<Player>& player,
    std::vector<std::shared_ptr<Enemy>>& enemies) const {
    ::clear(); // Clear the screen before drawing the new frame.
    // Print movement instructions.
    ::mvprintw(0, 0, "Use w,a,s,d to move. Press q to quit.");

    // Print player.
    ::attron(COLOR_PAIR(ClrPr::PLAYER_PAIR));
    ::mvprintw(player->cur_pos.first, player->cur_pos.second, "%c",
               player->ICON);
    ::attroff(COLOR_PAIR(ClrPr::PLAYER_PAIR));

    // Print enemies.
    ::attron(COLOR_PAIR(ClrPr::ENEMY_PAIR));
    for (const std::shared_ptr<Enemy>& e : enemies) {
        ::mvprintw(e->cur_pos.first, e->cur_pos.second, "%c", e->ICON);
    }
    ::attroff(COLOR_PAIR(ClrPr::ENEMY_PAIR));

    ::refresh();
}

position NcursesScreen::handle_input(position cur_pos, bool& is_running) const {
    // Update player's position according to the provided input.
    switch (int key_pressed = ::getch(); key_pressed) {
    case 'w':
        cur_pos.first--;
        return cur_pos;
    case 's':
        cur_pos.first++;
        return cur_pos;
    case 'a':
        cur_pos.second--;
        return cur_pos;
    case 'd':
        cur_pos.second++;
        return cur_pos;
    case 'q':
        is_running = false;
        return cur_pos;
    default:
        return cur_pos;
    }
}

void NcursesScreen::sleep_until_next_frame() const {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(refresh_interval_ms_));
}
