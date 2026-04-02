#include "mapstate.h"
#include "entities.h"
#include "ncurses_screen.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <memory>
#include <mutex>
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
    // No other threads run during construction so we may access player
    map[player->cur_pos.first][player->cur_pos.second] = player;

    for (int i = 0; i < difficulty * 5; i++) {
        add_enemy(std::make_shared<Goblin>(i, i));
    }

    logfile.open("run.log");

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
        std::size_t i = 0;
        while (i < enemies.size()) {
            auto entity = enemies[i];

            auto shot = pop_shot();
            if (shot != nullptr) {
                add_enemy(shot);
            }

            // projectiles move in a straight line and attack anything they hit
            if (auto* proj = dynamic_cast<Projectile*>(entity.get())) {
                logfile << std::format("rl {:p} Moving Proj\n",
                                       static_cast<void*>(entity.get()));
                bool alive = move_projectile(i, proj);
                if (alive) {
                    ++i;
                }
                continue;
            }

            if (entity->cur_hp <= 0) {
                logfile << std::format("rl {:p} Removing Dead\n",
                                       static_cast<void*>(entity.get()));
                remove_enemy(i);
                continue;
            }
            std::pair<Action, position> action;
            // Make the enemy act. We acquire the lock so that the enemy
            // always gets the most up-to-date info.
            action = entity->decide_action(player->get_pos(state_mutex));

            switch (action.first) {
            case Action::Move:
                logfile << std::format("rl {:p} Move\n",
                                       static_cast<void*>(entity.get()));
                move_enemy(action.second, entity);
                break;
            case Action::Attack:
                logfile << std::format("rl {:p} Attack\n",
                                       static_cast<void*>(entity.get()));
                attack_pos(action.second, entity->DAMAGE);
                break;
            }

            ++i;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
void MapState::add_enemy(std::shared_ptr<Enemy> entity) {
    std::scoped_lock lock(state_mutex);
    if (is_occupied(entity->cur_pos)) {
        return;
    }
    map[entity->cur_pos.first][entity->cur_pos.second] = entity;
    enemies.push_back(entity);
}

void MapState::remove_enemy(std::size_t entity_idx) {
    std::scoped_lock lock(state_mutex);
    auto e = enemies[entity_idx];
    if (0 < e->cur_hp) {
        return;
    }
    enemies[entity_idx] = std::move(enemies[enemies.size() - 1]);
    enemies.pop_back();

    map[e->cur_pos.first][e->cur_pos.second].reset();
}

bool MapState::move_projectile(size_t index, Projectile* proj) {
    bool alive = true;

    for (uint8_t mov = 0; mov < proj->MOVE_SPEED && alive; ++mov) {
        position cur = proj->get_pos(state_mutex);
        int next_row =
            static_cast<int>(cur.first) + static_cast<int>(proj->delta.first);
        int next_col =
            static_cast<int>(cur.second) + static_cast<int>(proj->delta.second);

        position next{static_cast<int8_t>(next_row),
                      static_cast<int8_t>(next_col)};

        // if the projectile moves out of bounds, remove it
        if (is_out_of_bounds(next)) {
            remove_enemy(index);
            alive = false;
            break;
        }

        // if projectile hits something, deal damage and remove projectile
        if (is_occupied(next)) {
            attack_pos(next, proj->DAMAGE);
            remove_enemy(index);
            alive = false;
            break;
        }

        auto e = enemies[index].get();
        // Empty tile: move the projectile forward.
        if (!move_to_pos(e, next)) {
            alive = false;
            remove_enemy(index);
            break;
        }
    }

    return alive;
}

// Check if the position is out of bounds. No lock is needed for this, since
// the bounds of the map never change.
bool MapState::is_out_of_bounds(position pos) const {
    return (pos.first < 0) || (ROWS <= pos.first) || (pos.second < 0) ||
           (COLUMNS <= pos.second);
}

// Check if the position is occupied. Every time we call this, we assume we're
// already holding the lock.
bool MapState::is_occupied(position pos) const {
    if (is_out_of_bounds(pos)) {
        return true;
    }

    return map[(pos.first)][(pos.second)] != nullptr;
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
        move_to_pos(enemy.get(), new_pos);
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

bool MapState::attack_pos(position pos, uint8_t dmg, uint8_t radius) {
    if (radius != 1) {
        return false; // TODO: Add radius calculations
    }

    if (is_out_of_bounds(pos)) {
        return false;
    }

    std::scoped_lock lock(state_mutex);
    auto& entity = map[pos.first][pos.second];
    if (entity == nullptr) {
        return false;
    }
    auto remaining_hp = entity->take_damage(dmg);

    // If the player dies, end the game.
    if (remaining_hp <= 0 && entity.get() == player.get()) {
        game_running = false;
    }

    return true;
}

void MapState::move_player(position goal_pos) {
    move_to_pos(player.get(), goal_pos);
}

bool MapState::move_to_pos(Entity* entity, position new_pos) {
    position old_pos = entity->get_pos(state_mutex);

    {
        std::scoped_lock lock(state_mutex);

        if (is_occupied(new_pos)) {
            return false;
        }

        std::swap(map[old_pos.first][old_pos.second],
                  map[new_pos.first][new_pos.second]);
    }

    entity->set_pos(state_mutex, new_pos);

    return true;
}

void MapState::handle_shot(position shoot_delta) {
    // First tile in the shooting direction is in front of the player
    position start = player->get_pos(state_mutex);
    start.first += shoot_delta.first;
    start.second += shoot_delta.second;

    // ignore shots that would start outside the map
    if (is_out_of_bounds(start)) {
        return;
    }

    // attack the first tile. if it stays empty, spawn a projectile
    bool hit = attack_pos(start, player->DAMAGE);
    if (hit) {
        return;
    }

    if (!is_occupied(start)) {
        push_shot(
            std::make_shared<Projectile>(start, shoot_delta, player->DAMAGE));
    }
}
std::shared_ptr<Projectile> MapState::pop_shot() {
    std::scoped_lock lock(queue_mutex);
    if (shot_queue.empty()) {
        return nullptr;
    }
    auto ret = shot_queue.front();
    shot_queue.pop();
    return ret;
}

void MapState::push_shot(std::shared_ptr<Projectile> shot) {
    std::scoped_lock lock(queue_mutex);
    shot_queue.push(shot);
}

// TODO setja inní functions. Kalla alltaf á get pos eða set pos ef þarf í
// staðinn fyrir að kóða mitt eigið.
// TODO if I need an occupancy check. scoped lock, then call is_occupied()
void MapState::ncurses_thread() {
    while (game_running.load()) {
        position cur_pos;
        // Try to acquire the lock on the state. If we can't, just skip this
        // frame. This is so that we don't block the main thread if the player
        // is in the middle of moving or attacking.
        cur_pos = player->get_pos(state_mutex);

        // Handle input and get new position or shoot. If the player wants to
        // quit, then break.
        position shoot_delta;
        bool did_shoot = false;
        auto new_pos =
            SCREEN.handle_input(cur_pos, game_running, shoot_delta, did_shoot);
        if (!game_running.load())
            break;

        if (did_shoot) {
            logfile << "nc Shooting\n";
            handle_shot(shoot_delta);
        }

        logfile << "nc Moving Player\n";
        move_player(new_pos);
        // Render the frame. We acquire the lock to make sure that we always
        // render the true state of the game, even if something else is being
        // updated. This might cause some stuttering, but it's better than
        // rendering something that isn't true.
        SCREEN.render_frame(player, enemies, ROWS, COLUMNS);
        SCREEN.sleep_until_next_frame();
    }
}
