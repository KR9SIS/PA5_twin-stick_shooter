#include "entities.h"
#include <cstdint>

position Entity::get_pos() {
    return cur_pos;
}

void Entity::set_pos(position new_pos) {
    cur_pos = new_pos;
}

void Entity::change_health(int8_t dmg) {
    cur_hp += dmg;
}
