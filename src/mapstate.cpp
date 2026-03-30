#include "mapstate.h"
#include "ncurses_screen.h"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <ncurses.h>
#include <thread>
#include <utility>

MapState::MapState(size_t r, size_t c, uint8_t difficulty,
                   const NcursesScreen& screen)
    : ROWS(r), COLUMNS(c), DIFFICULTY(difficulty), SCREEN(screen),
      game_running(true) {
    map.resize(r);
    for (auto& row : map) {
        row.resize(c);
    }
    player = std::make_shared<Player>(r / 2, c / 2);
    map[player->cur_pos.first][player->cur_pos.second] = player;

    for (int i = 0; i < difficulty * 5; i++) {
        enemies.push_back(std::make_shared<Goblin>(i, i));
        map[i][i] = enemies.back();
    }

    std::thread ncurses_thread(&MapState::ncurses_thread, this);
    ncurses_thread.detach();
}

void MapState::run_level() {
    while (game_running) {
        for (auto& enemy : enemies) {
            auto action = enemy->act(player->get_pos());
            switch (action.first) {
            case Action::Move:
                move_enemy(action.second, enemy);
                break;
            case Action::Attack:
                attack_pos(action.second, enemy->DAMAGE);
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

bool MapState::occupied(position pos) {
    if (0 <= pos.first && pos.first < ROWS && 0 <= pos.second &&
        pos.second < COLUMNS) {
        return map[pos.first][pos.second] != nullptr;
    }
    return true;
}

void MapState::move_enemy(position goal_pos, std::shared_ptr<Enemy>& enemy) {
    auto move = [this, &enemy](int8_t dir, bool is_row) {
        // get new position
        // check if new position is occupied
        // if it is continue
        // else swap cur_pos and new_pos

        auto cur_pos = enemy->get_pos();
        position new_pos = cur_pos;

        if (is_row) {
            new_pos.first += (dir > 0) ? 1 : -1;
        } else {
            new_pos.second += (dir > 0) ? 1 : -1;
        }

        if (occupied(new_pos)) {
            return;
        }

        // WARN: RACE CONDITION
        std::swap(map[cur_pos.first][cur_pos.second],
                  map[new_pos.first][new_pos.second]);

        enemy->set_pos(new_pos);
    };
    for (uint8_t mov = 0; mov < enemy->MOVE_SPEED; mov++) {
        auto dir = std::make_pair(goal_pos.first - enemy->cur_pos.first,
                                  goal_pos.second - enemy->cur_pos.second);
        if (dir.first && dir.second) {
            if (rand() % 2) {
                move(dir.first, true);
            } else {
                move(dir.second, false);
            }

        } else if (dir.first) {
            move(dir.first, true);

        } else if (dir.second) {
            move(dir.second, false);
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
    entity->change_health(-dmg);
}

void MapState::move_player(position goal_pos) {
    if (occupied(goal_pos)) {
        return;
    }
    // WARN: RACE CONDITION
    std::swap(map[player->cur_pos.first][player->cur_pos.second],
              map[goal_pos.first][goal_pos.second]);

    player->set_pos(goal_pos);
}

void MapState::ncurses_thread() {
    while (game_running) {
        auto new_pos = SCREEN.handle_input(player->get_pos(), game_running);
        move_player(new_pos);
        SCREEN.render_frame(player, enemies);
        SCREEN.sleep_until_next_frame();
    }
}
