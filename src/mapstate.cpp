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

    ncurses_worker = std::thread(&MapState::ncurses_thread, this);
}

MapState::~MapState() {
    game_running.store(false); // Make the atomic_bool be false.
    // If the ncurses thread is still running, join it.
    if (ncurses_worker.joinable()) {
        ncurses_worker.join();
    }
}

void MapState::run_level() {
    // Load the current state of the game_running atomic_bool.
    while (game_running.load()) {
        for (auto& enemy : enemies) {
            std::pair<Action, position> action;
            // Make the enemy act. We acquire the lock so that the enemy always
            // gets the most up-do-date info.
            {
                std::scoped_lock lock(state_mutex);
                action = enemy->act(player->get_pos());
            }
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

// Private version. Every time we call this, we assume we're already holding
// the lock.
bool MapState::occupied_unlocked(position pos) const {
    if (0 <= pos.first && pos.first < ROWS && 0 <= pos.second &&
        pos.second < COLUMNS) {
        return map[pos.first][pos.second] != nullptr;
    }
    return true;
}

// Public version. Every time we call this, we *don't* assume we're holding the
// lock, so we acquire it here.
bool MapState::occupied(position pos) {
    std::scoped_lock lock(state_mutex); // Acquire the lock.
    return occupied_unlocked(pos);      // Call the unlocked version.
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

        if (occupied_unlocked(new_pos)) {
            return;
        }

        // WARN: RACE CONDITION (Might be resolved now).
        std::swap(map[cur_pos.first][cur_pos.second],
                  map[new_pos.first][new_pos.second]);

        enemy->set_pos(new_pos);
    };
    std::scoped_lock lock(state_mutex); // Acquire the lock on the state.
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

    std::scoped_lock lock(state_mutex);
    auto& entity = map[pos.first][pos.second];
    if (entity == nullptr) {
        return;
    }
    auto remaining_hp = entity->take_damage(dmg);

    //end game if player is dead
    if (entity.get() == player.get() && remaining_hp <= 0) {
        game_running = false;
    }
}

void MapState::move_player(position goal_pos) {
    std::scoped_lock lock(state_mutex);
    if (occupied_unlocked(goal_pos)) {
        return;
    }
    std::swap(map[player->cur_pos.first][player->cur_pos.second],
              map[goal_pos.first][goal_pos.second]);

    player->set_pos(goal_pos);
}

void MapState::ncurses_thread() {
    while (game_running.load()) {
        position cur_pos;
        // Try to acquire the lock on the state. If we can't, just skip this
        // frame. This is so that we don't block the main thread if the player
        // is in the middle of moving or attacking.
        {
            std::scoped_lock lock(state_mutex);
            cur_pos = player->get_pos();
        }

        // Handle input and get new position. If the player wants to quit,
        // then break.
        auto new_pos = SCREEN.handle_input(cur_pos, game_running);
        if (!game_running.load()) {
            break;
        }

        move_player(new_pos);
        // Render the frame. We acquire the lock to make sure that we always
        // render the true state of the game, even if something else is being
        // updated. This might cause some stuttering, but it's better than
        // rendering something that isn't true.
        {
            std::scoped_lock lock(state_mutex);
            SCREEN.render_frame(player, enemies, ROWS, COLUMNS);
        }
        SCREEN.sleep_until_next_frame();
    }
}
