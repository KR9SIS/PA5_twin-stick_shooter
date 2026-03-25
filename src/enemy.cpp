#include "entities.h"
#include <cstdint>
#include <cstdlib>
#include <utility>

std::pair<Action, position> Enemy::act(position player_pos) {
    int8_t x_dir = player_pos.first - cur_pos.first;
    int8_t y_dir = player_pos.second - cur_pos.second;
    if ((x_dir == 0 || y_dir == 0) && std::abs(x_dir + y_dir) == 1) {
        return std::make_pair(Action::Attack, player_pos);
    }
    return std::make_pair(Action::Move, player_pos);
}
