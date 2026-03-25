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

    MapState(size_t rows, size_t columns);

    void get_input();
  };
