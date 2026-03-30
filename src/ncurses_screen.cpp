#include "ncurses_screen.h"

#include <chrono>
#include <memory>
#include <ncurses.h>
#include <thread>
#include <vector>

namespace {
enum ClrPr { PLAYER_PAIR = 1, ENEMY_PAIR = 2 };
constexpr int BATTLE_WINDOW_START_ROW = 1;
constexpr int BATTLE_WINDOW_START_COLUMN = 0;
} // namespace

NcursesScreen::NcursesScreen(int frames_per_second)
    : refresh_interval_ms_(1000 / frames_per_second) {
    ::initscr(); // Init ncurses library and set up the screen for drawing.
    ::cbreak();  // Disable line buffering, so input is sent immediately.
    ::noecho();  // Don't print user input to the screen.
    ::keypad(::stdscr, TRUE);  // Enable special keys, e.g. arrow keys.
    ::curs_set(0);             // Hide the cursor.
    ::nodelay(::stdscr, TRUE); // Don't block on user input.

    ::start_color();        // Enable color functionality in ncurses.
    ::use_default_colors(); // Allow -1 to mean terminal default background.
    ::init_pair(ClrPr::PLAYER_PAIR, COLOR_CYAN, -1);
    ::init_pair(ClrPr::ENEMY_PAIR, COLOR_RED, -1);
}

NcursesScreen::~NcursesScreen() {
    if (battle_window_ != nullptr) {
        ::delwin(battle_window_);
        battle_window_ = nullptr;
    }
    ::endwin(); // Restore the terminal to its original state after exiting.
}

void NcursesScreen::setup_battle_window(int8_t rows, int8_t columns) const {
    if (battle_window_ != nullptr && battle_rows_ == rows &&
        battle_columns_ == columns) {
        return;
    }

    if (battle_window_ != nullptr) {
        ::delwin(battle_window_);
    }

    // +2 makes room for the border while preserving map coordinates inside.
    battle_window_ = ::newwin(rows + 2, columns + 2, BATTLE_WINDOW_START_ROW,
                              BATTLE_WINDOW_START_COLUMN);
    battle_rows_ = rows;
    battle_columns_ = columns;
}

void NcursesScreen::render_frame(
    const std::shared_ptr<Player>& player,
    const std::vector<std::shared_ptr<Enemy>>& enemies, int8_t rows,
    int8_t columns) const {
    ::erase(); // Clear the screen before drawing the new frame.
    // Print movement instructions.
    ::mvprintw(0, 0, "Use w,a,s,d to move. Press q to quit.");

    setup_battle_window(rows, columns); // Create the battle window.
    ::werase(battle_window_);           // Erase the previous battle window.
    ::box(battle_window_, 0, 0); // Draw a border around the battle window.

    // Print player.
    ::wattron(battle_window_, COLOR_PAIR(ClrPr::PLAYER_PAIR));
    mvwaddch(battle_window_, player->cur_pos.first + 1,
             player->cur_pos.second + 1, player->ICON);
    ::wattroff(battle_window_, COLOR_PAIR(ClrPr::PLAYER_PAIR));

    // Print enemies.
    ::wattron(battle_window_, COLOR_PAIR(ClrPr::ENEMY_PAIR));
    for (const std::shared_ptr<Enemy>& e : enemies) {
        mvwaddch(battle_window_, e->cur_pos.first + 1, e->cur_pos.second + 1,
                 e->ICON);
    }
    ::wattroff(battle_window_, COLOR_PAIR(ClrPr::ENEMY_PAIR));

    ::wnoutrefresh(::stdscr);       // Refresh standard screen (instructions).
    ::wnoutrefresh(battle_window_); // Refresh battle window.
    ::doupdate();                   // Update the screen.
}

position NcursesScreen::handle_input(position cur_pos,
                                     std::atomic_bool& is_running) const {
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
