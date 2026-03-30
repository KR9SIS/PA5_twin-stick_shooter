#include "entities.h"

Player::Player() : Entity(10, 1, 1, 0, 0) {}

int8_t Player::take_damage(int8_t dmg) {
    change_health(-dmg);
    if (cur_hp < 0) {
        cur_hp = 0;
    }
    return cur_hp;
}