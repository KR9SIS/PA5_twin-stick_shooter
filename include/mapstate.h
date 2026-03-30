#include "entities.h"
#include "ncurses_screen.h"
#include <cstdint>
#include <memory>
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
    bool game_running;

    MapState(size_t rows, size_t columns, uint8_t difficulty,
             const NcursesScreen& screen);

    void run_level();
    void get_input();
    void move_enemy(position goal_pos, std::unique_ptr<Enemy>& enemy);

    void attack_pos(position pos, uint8_t dmg, uint8_t radius = 1);

    bool occupied(position pos);

    void handle_input();
};
