#pragma once

class NcursesScreen final {
  public:
    explicit NcursesScreen(int frames_per_second);
    ~NcursesScreen();

    NcursesScreen(const NcursesScreen&) = delete;
    NcursesScreen& operator=(const NcursesScreen&) = delete;

    void render_frame(int player_y, int player_x, int enemy_y,
                      int enemy_x) const;
    void handle_input(int& player_y, int& player_x, bool& is_running) const;
    void sleep_until_next_frame() const;

  private:
    int refresh_interval_ms_;
};
