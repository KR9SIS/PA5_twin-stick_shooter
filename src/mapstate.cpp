#include "mapstate.h"
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>

MapState::MapState(size_t r, size_t c, uint8_t difficulty)

    : ROWS(r), COLUMNS(c), DIFFICULTY(difficulty),
      map(r, std::vector<std::unique_ptr<Entity>>(c)) {
    game_running = true;
}

void MapState::run_level() {
    while (game_running) {
        for (auto& enemy : enemies) {
            auto action = enemy.act(player.get_pos());
            switch (action.first) {
            case Action::Move:
                enemy.set_pos(move_entity(action.second));
                break;
            case Action::Attack:
                attack_pos(action.second);
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
