#include "mapstate.h"
#include <chrono>
#include <thread>

MapState::MapState(size_t r, size_t c)

    : difficulty(difficulty), ROWS(r), COLUMNS(c),
      map(r, std::vector<std::unique_ptr<Entity>>(c)) {
    game_running = true;
}

void MapState::run_level(int difficulty) {
    // while (game is running) { for (auto &e : enemies) e.act(); /* sleep */ }
    while (game_running) {
        for (auto& enemy : enemies) {
            enemy.act();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500))
    }
}
