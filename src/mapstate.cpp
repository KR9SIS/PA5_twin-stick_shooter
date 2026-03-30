#include "mapstate.h"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <thread>
#include <utility>

MapState::MapState(size_t r, size_t c, uint8_t difficulty)
    : ROWS(r), COLUMNS(c), DIFFICULTY(difficulty), game_running(true) {
    map.resize(r);
    for (auto& row : map) {
        row.resize(c);
    }
}

void MapState::run_level() {
    while (game_running) {
        for (auto& enemy : enemies) {
            auto action = enemy.act(player.get_pos());
            switch (action.first) {
            case Action::Move:
                move_enemy(action.second, enemy);
                break;
            case Action::Attack:
                attack_pos(action.second, enemy.damage);
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
bool MapState::occupied(position pos) {
    return map[pos.first][pos.second] != nullptr;
}

void MapState::move_enemy(position goal_pos, Enemy& enemy) {
    auto cur_pos = enemy.get_pos();

    auto move = [this, &cur_pos](int8_t dir) {
        // get new position
        // check if new position is occupied
        // if it is continue
        // else swap cur_pos and new_pos

        position new_pos = cur_pos;
        new_pos.first += (dir > 0) ? 1 : -1;

        if (occupied(new_pos)) {
            return;
        }
        std::swap(map[cur_pos.first][cur_pos.second],
                  map[new_pos.first][new_pos.second]);
    };
    for (uint8_t mov = 0; mov < enemy.move_speed; mov++) {
        auto dir = std::make_pair(goal_pos.first - cur_pos.first,
                                  goal_pos.second - cur_pos.second);
        if (dir.first && dir.second) {
            if (rand() % 2) {
                move(dir.first);
            } else {
                move(dir.second);
            }

        } else if (dir.first) {
            move(dir.first);

        } else if (dir.second) {
            move(dir.second);
        }
    }
}

void MapState::attack_pos(position pos, uint8_t dmg, uint8_t radius) {
    if (radius != 1) {
        return; // TODO: Add radius calculations
    }

    auto& entity = map[pos.first][pos.second];
    if (entity == nullptr) {
        return;
    }
    entity->take_damage(dmg);
}
