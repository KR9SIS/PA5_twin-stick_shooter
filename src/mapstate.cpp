#include "mapstate.hpp"

MapState::MapState(size_t r, size_t c)
    : ROWS(r), COLUMNS(c), map(r, std::vector<std::unique_ptr<Entity>>(c)) {}
