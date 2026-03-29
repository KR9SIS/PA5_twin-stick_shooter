#include "ncurses_screen.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <ncurses.h>
#include <thread>
#include <vector>

namespace {
enum ClrPr { PLAYER_PAIR = 1, ENEMY_PAIR = 2 };
}

NcursesScreen::NcursesScreen(int frames_per_second)
    : refresh_interval_ms_(1000 / frames_per_second) {
    ::initscr();
    ::cbreak();
    ::noecho();
    ::keypad(::stdscr, TRUE);
    ::curs_set(0);
    ::nodelay(::stdscr, TRUE);

    ::start_color();
    ::init_pair(ClrPr::PLAYER_PAIR, COLOR_CYAN, COLOR_BLACK);
    ::init_pair(ClrPr::ENEMY_PAIR, COLOR_RED, COLOR_BLACK);
}

NcursesScreen::~NcursesScreen() {
    ::endwin();
}

void NcursesScreen::render_frame(
    const Player& player, std::vector<std::unique_ptr<Enemy>>& enemies) const {
    ::clear();
    ::mvprintw(0, 0, "Use w,a,s,d to move. Press q to quit.");

    ::attron(COLOR_PAIR(ClrPr::PLAYER_PAIR));
    ::mvprintw(player.cur_pos.first, player.cur_pos.second, "@");
    ::attroff(COLOR_PAIR(ClrPr::PLAYER_PAIR));

    ::attron(COLOR_PAIR(ClrPr::ENEMY_PAIR));
    for (const std::unique_ptr<Enemy>& e : enemies) {
        ::mvprintw(e->cur_pos.first, e->cur_pos.second, "%c", e->ICON);
    }
    ::attroff(COLOR_PAIR(ClrPr::ENEMY_PAIR));

    ::refresh();
}

void NcursesScreen::handle_input(int8_t& player_y, int8_t& player_x,
                                 bool& is_running) const {
    switch (int key_pressed = ::getch(); key_pressed) {
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
}

void NcursesScreen::sleep_until_next_frame() const {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(refresh_interval_ms_));
}
