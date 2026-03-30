#include "entities.h"
#include <atomic>
#include <memory>
#include <ncurses.h>
#include <vector>
#pragma once

class NcursesScreen final {
  public:
    explicit NcursesScreen(int frames_per_second);
    ~NcursesScreen();

    NcursesScreen(const NcursesScreen&) = delete;
    NcursesScreen& operator=(const NcursesScreen&) = delete;

    void render_frame(const std::shared_ptr<Player>& player,
                      const std::vector<std::shared_ptr<Enemy>>& enemies,
                      int8_t rows, int8_t columns) const;
    position handle_input(position cur_pos, std::atomic_bool& is_running) const;
    void sleep_until_next_frame() const;

  private:
    void setup_battle_window(int8_t rows, int8_t columns) const;

    int refresh_interval_ms_;
    mutable WINDOW* battle_window_ = nullptr;
    mutable int8_t battle_rows_ = -1;
    mutable int8_t battle_columns_ = -1;
};
