#include "mapstate.h"
#include "ncurses_screen.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
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
    map[player->get_pos(state_mutex).first]
       [player->get_pos(state_mutex).second] = player;

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
        // Remove any enemies/projectiles whose HP has dropped to 0.
        {
            std::scoped_lock lock(state_mutex);
            enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                                         [](const std::shared_ptr<Enemy>& e) {
                                             return e->cur_hp <= 0;
                                         }),
                           enemies.end());
        }

        std::size_t i = 0;
        while (i < enemies.size()) {
            auto& enemy = enemies[i];

            // irojectiles move in a straight line and attack anything they hit
            if (auto* proj = dynamic_cast<Projectile*>(enemy.get())) {
                bool alive = true;
                for (uint8_t mov = 0; mov < enemy->MOVE_SPEED && alive; ++mov) {
                    position cur = proj->get_pos();
                    int next_row = static_cast<int>(cur.first) +
                                   static_cast<int>(proj->delta.first);
                    int next_col = static_cast<int>(cur.second) +
                                   static_cast<int>(proj->delta.second);

                    // if the projectile moves out of bounds, remove it
                    if (next_row < 0 || next_row >= ROWS || next_col < 0 ||
                        next_col >= COLUMNS) {
                        {
                            std::scoped_lock lock(state_mutex);
                            map[cur.first][cur.second].reset();
                        }
                        enemies.erase(enemies.begin() + i);
                        alive = false;
                        break;
                    }

                    position next{static_cast<int8_t>(next_row),
                                  static_cast<int8_t>(next_col)};

                    std::shared_ptr<Entity> target;
                    {
                        std::scoped_lock lock(state_mutex);
                        target = map[next.first][next.second];
                    }

                    // if projectile hits something, deal damage and remove projectile
                    if (target != nullptr) {
                        attack_pos(next, enemy->DAMAGE);
                        {
                            std::scoped_lock lock(state_mutex);
                            map[cur.first][cur.second].reset();
                        }
                        enemies.erase(enemies.begin() + i);
                        alive = false;
                        break;
                    }

                    // Empty tile: move the projectile forward.
                    if (move_pos(cur, next)) {
                        proj->set_pos(next);
                    } else {
                        // If it can't move for some reason, remove it.
                        enemies.erase(enemies.begin() + i);
                        alive = false;
                        break;
                    }
                }

                if (alive) {
                    ++i;
                }
            } else {
                std::pair<Action, position> action;
                // Make the enemy act. We acquire the lock so that the enemy always
                // gets the most up-to-date info.
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

                ++i;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

// Private version. Every time we call this, we assume we're already holding
// the lock.
bool MapState::occupied(position pos) const {
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

        auto cur_pos = enemy->get_pos(state_mutex);
        position new_pos = cur_pos;

        if (is_row) {
            new_pos.first += (dir > 0) ? 1 : -1;
        } else {
            new_pos.second += (dir > 0) ? 1 : -1;
        }
        if (move_pos(cur_pos, new_pos)) {
            enemy->set_pos(state_mutex, new_pos);
        }
    };
    auto cur_pos = enemy->get_pos(state_mutex);
    for (uint8_t mov = 0; mov < enemy->MOVE_SPEED; mov++) {
        auto dir = std::make_pair(goal_pos.first - cur_pos.first,
                                  goal_pos.second - cur_pos.second);
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

    if (remaining_hp <= 0) {
        // If the player dies, end the game.
        if (entity.get() == player.get()) {
            game_running = false;
        }
        entity.reset();
    }
}

void MapState::move_player(position goal_pos) {
    if (move_pos(player->get_pos(state_mutex), goal_pos)) {
        player->set_pos(state_mutex, goal_pos);
    }
}

bool MapState::move_pos(position old_pos, position new_pos) {
    std::scoped_lock lock(state_mutex);
    if (occupied(new_pos)) {
        return false;
    }
    std::swap(map[old_pos.first][old_pos.second],
              map[new_pos.first][new_pos.second]);
    return true;
}

void MapState::ncurses_thread() {
    while (game_running.load()) {
        position cur_pos;
        // Try to acquire the lock on the state. If we can't, just skip this
        // frame. This is so that we don't block the main thread if the player
        // is in the middle of moving or attacking.
        cur_pos = player->get_pos(state_mutex);

        // Handle input and get new position or shoot. If the player wants to quit,
        // then break.
        position shoot_delta;
        bool did_shoot = false;
        auto new_pos = SCREEN.handle_input(cur_pos, game_running, shoot_delta, did_shoot);
        if (!game_running.load()) break;

        if (did_shoot) {
            // First tile in the shooting direction is in front of the player
            position start = player->get_pos();
            start.first += shoot_delta.first;
            start.second += shoot_delta.second;

            // ignore shots that would start outside the map
            if (0 <= start.first && start.first < ROWS && 0 <= start.second &&
                start.second < COLUMNS) {
                // attack the first tile. if it stays empty, spawn a projectile
                attack_pos(start, player->DAMAGE);

                std::shared_ptr<Entity> cell_after;
                {
                    std::scoped_lock lock(state_mutex);
                    cell_after = map[start.first][start.second];
                }

                if (cell_after == nullptr) {
                    auto proj = std::make_shared<Projectile>(start, shoot_delta, player->DAMAGE);
                    std::scoped_lock lock(state_mutex);
                    enemies.push_back(proj);
                    map[proj->cur_pos.first][proj->cur_pos.second] = proj;
                }
            }
        }

        move_player(new_pos);
        // Render the frame. We acquire the lock to make sure that we always
        // render the true state of the game, even if something else is being
        // updated. This might cause some stuttering, but it's better than
        // rendering something that isn't true.
        SCREEN.render_frame(player, enemies, ROWS, COLUMNS);
        SCREEN.sleep_until_next_frame();
    }
}
