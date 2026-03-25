#include "entities.hpp"
#include <memory>
#include <vector>

class MapState {
  public:
    std::vector<Enemy> enemies;
    Player player;
    const size_t ROWS;
    const size_t COLUMNS;
    std::vector<std::vector<std::unique_ptr<Entity>>> map;
    int difficulty;
    bool game_running;
    

    MapState(size_t rows, size_t columns);

    void get_input();
    std::pair<uint8_t, uint8_t> move_entity(MoveDir direction);
  };
