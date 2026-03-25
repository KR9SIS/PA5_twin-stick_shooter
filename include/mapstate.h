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
    bool game_running;

    MapState(size_t rows, size_t columns, uint8_t difficulty);

    void run_level();
    void get_input();
    std::pair<uint8_t, uint8_t> move_entity(std::pair<uint8_t, uint8_t> pos);

    void attack_pos(std::pair<uint8_t, uint8_t> pos, uint8_t radius = 1);
};
