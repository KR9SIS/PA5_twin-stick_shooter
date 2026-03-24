#include "entities.hpp"

Player::Player() : Entity(10, 1, 1, 0, 0) {}

void Player::act(Action a) {
    (void)a;
}

void Player::attack() {}

int8_t Player::take_damage(int8_t dmg) {
    cur_hp -= dmg;
    return cur_hp;
}

void Player::update() {}
