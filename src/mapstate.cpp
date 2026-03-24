#include "mapstate.hpp"

MapState::MapState(size_t r, size_t c) : ROWS(r), COLUMNS(c), map(r) {
    for (auto& row : map) {
        row.resize(c);
    }
}
