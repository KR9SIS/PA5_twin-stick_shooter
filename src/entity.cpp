#include "entities.h"
#include <cstdint>
#include <utility>

std::pair<uint8_t, uint8_t> Entity::get_pos() {
    return cur_pos;
}

void Entity::set_pos(std::pair<uint8_t, uint8_t> new_pos) {
    cur_pos = new_pos;
}
