#include "entities.h"
#include <cstdint>
#include <memory>
#include <vector>
#pragma once

class NcursesScreen final {
  public:
    explicit NcursesScreen(int frames_per_second);
    ~NcursesScreen();

    NcursesScreen(const NcursesScreen&) = delete;
    NcursesScreen& operator=(const NcursesScreen&) = delete;

    void render_frame(const Player& player,
                      std::vector<std::unique_ptr<Enemy>>& enemies) const;
    void handle_input(int8_t& player_y, int8_t& player_x,
                      bool& is_running) const;
    void sleep_until_next_frame() const;

  private:
    int refresh_interval_ms_;
};
