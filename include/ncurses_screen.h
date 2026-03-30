#include "entities.h"
#include <atomic>
#include <memory>
#include <vector>
#pragma once

class NcursesScreen final {
  public:
    explicit NcursesScreen(int frames_per_second);
    ~NcursesScreen();

    NcursesScreen(const NcursesScreen&) = delete;
    NcursesScreen& operator=(const NcursesScreen&) = delete;

    void render_frame(const std::shared_ptr<Player>& player,
                      const std::vector<std::shared_ptr<Enemy>>& enemies) const;
    position handle_input(position cur_pos, std::atomic_bool& is_running) const;
    void sleep_until_next_frame() const;

  private:
    int refresh_interval_ms_;
};
