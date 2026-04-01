#pragma once

#include "entities.h"
#include "ncurses_screen.h"
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class MapState {
  public:
    std::vector<std::shared_ptr<Enemy>> enemies;
    std::shared_ptr<Player> player;
    const int8_t ROWS;
    const int8_t COLUMNS;
    const uint8_t DIFFICULTY;
    NcursesScreen& SCREEN;
    std::vector<std::vector<std::shared_ptr<Entity>>> map;
    int difficulty;

    MapState(size_t rows, size_t columns, uint8_t difficulty,
             NcursesScreen& screen);
    ~MapState();

    void run_level();
    void move_to_pos(Entity* entity, position new_pos);
    void move_enemy(position goal_pos, std::shared_ptr<Enemy>& enemy);
    void move_player(position goal_pos);
    void attack_pos(position pos, uint8_t dmg, uint8_t radius = 1);
    void ncurses_thread();

  private:
    // TODO: Make TestBullet a proper Entity.
    struct TestBullet {
        position pos{};
        position pos_diff{};
        char icon = '*';
        std::chrono::steady_clock::time_point last_moved_at{};
    };

    static constexpr auto PLAYER_MOVE_INTERVAL = std::chrono::milliseconds(70);
    static constexpr auto BULLET_FIRE_INTERVAL = std::chrono::milliseconds(120);
    static constexpr auto BULLET_STEP_INTERVAL = std::chrono::milliseconds(45);

    bool is_out_of_bounds(position pos) const;
    bool is_occupied(position pos) const;
    void update_player_from_input(const InputState& input_state,
                                  std::chrono::steady_clock::time_point now);
    void spawn_test_bullets(const InputState& input_state,
                            std::chrono::steady_clock::time_point now);
    void move_test_bullets_forward(std::chrono::steady_clock::time_point now);
    void create_render_state(EntityRenderData& player,
                             std::vector<EntityRenderData>& enemies_render_data,
                             std::vector<BulletRenderData>& bullets) const;
    static position get_dir_diff(InputDir direction);
    static char get_bullet_icon(InputDir direction);

    mutable std::mutex state_mutex;
    std::thread ncurses_worker;
    std::atomic_bool game_running;
    std::vector<TestBullet> test_bullets;
    std::chrono::steady_clock::time_point last_player_move_time_{};
    std::array<std::chrono::steady_clock::time_point, ::INPUT_DIRECTIONS_COUNT>
        last_bullet_fire_time_{};
};
