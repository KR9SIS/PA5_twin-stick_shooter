#include "entities.h"

position Entity::get_pos() {
    return cur_pos;
}

void Entity::set_pos(position new_pos) {
    cur_pos = new_pos;
}
