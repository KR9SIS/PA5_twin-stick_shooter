#include "entities.h"
#include <cstdint>
#include <memory>
#include <vector>

class MapState {
  public:
    std::vector<Enemy> enemies;
    Player player;
    const size_t ROWS;
    const size_t COLUMNS;
    const uint8_t DIFFICULTY;
    std::vector<std::vector<std::unique_ptr<Entity>>> map;
    int difficulty;
    bool game_running;

    MapState(size_t rows, size_t columns, uint8_t difficulty);

    void run_level();
    void get_input();
    void move_enemy(position goal_pos, Enemy& enemy);

    void attack_pos(position pos, uint8_t dmg, uint8_t radius = 1);

    bool occupied(position pos);
};
