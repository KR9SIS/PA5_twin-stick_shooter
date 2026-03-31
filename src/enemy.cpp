#include "entities.h"
#include <cstdint>
#include <cstdlib>
#include <utility>

std::pair<Action, position> Enemy::decide_action(position player_pos) {
    // Manhattan Distance Formula
    uint8_t dist = std::abs(player_pos.first - cur_pos.first) +
                   std::abs(player_pos.second - cur_pos.second);
    if (dist == 1) {
        return std::make_pair(Action::Attack, player_pos);
    }
    return std::make_pair(Action::Move, player_pos);
}
