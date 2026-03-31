#include "entities.h"
#include "ncurses_screen.h"
#include <atomic>
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
    const NcursesScreen& SCREEN;
    std::vector<std::vector<std::shared_ptr<Entity>>> map;
    int difficulty;

    MapState(size_t rows, size_t columns, uint8_t difficulty,
             const NcursesScreen& screen);
    ~MapState();

    void run_level();
    void get_input();

    void move_to_pos(Entity* entity, position new_pos);
    void move_enemy(position goal_pos, std::shared_ptr<Enemy>& enemy);
    void move_player(position goal_pos);

    void attack_pos(position pos, uint8_t dmg, uint8_t radius = 1);

    void ncurses_thread();

  private:
    bool occupied(position pos) const;

    std::mutex state_mutex;
    std::thread ncurses_worker;
    std::atomic_bool game_running;
};
