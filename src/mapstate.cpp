#include "mapstate.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace {
// Check if the given input_dir is active in the input state. If firing is
// true, check the firing state; otherwise, check the movement state.
bool is_dir_active(const InputState& input_state, InputDir input_dir,
                   bool firing) {
    const auto key_index = static_cast<size_t>(input_dir);

    // Check the appropriate input array based on whether we're checking firing
    // or movement. (E.g. “Is the player moving up?” vs “Is the player firing
    // up?”)
    if (firing) {
        return input_state.fire_key_held[key_index];
    } else {
        return input_state.move_key_held[key_index];
    }
}
} // namespace

MapState::MapState(size_t r, size_t c, uint8_t difficulty,
                   NcursesScreen& screen)
    : ROWS(r), COLUMNS(c), DIFFICULTY(difficulty), MAX_ENEMIES(DIFFICULTY / 4),
      SCREEN(screen), game_running(true) {

    map.resize(r);
    for (auto& row : map) {
        row.resize(c);
    }

    const auto now = std::chrono::steady_clock::now();
    last_player_move_time_ = now - PLAYER_MOVE_INTERVAL;
    last_bullet_fire_time_.fill(now - BULLET_FIRE_INTERVAL);

    player = std::make_shared<Player>(r / 2, c / 2);
    map[player->get_pos(state_mutex).first]
       [player->get_pos(state_mutex).second] = player;

    killed = 0;
    spawned = 0;
    directions.push_back(position(0, COLUMNS / 2));        // top mid
    directions.push_back(position(ROWS - 1, COLUMNS / 2)); // bottom mid
    directions.push_back(position(ROWS / 2, 0));           // mid left
    directions.push_back(position(ROWS / 2, COLUMNS - 1)); // mid right

    ncurses_worker = std::thread(&MapState::ncurses_thread, this);
}

MapState::~MapState() {
    game_running.store(false); // Make the atomic_bool be false.
    // If the ncurses thread is still running, join it.
    if (ncurses_worker.joinable()) {
        ncurses_worker.join();
    }
}

void MapState::remove_enemy(std::size_t i, std::shared_ptr<Enemy> enemy) {
    std::scoped_lock lock(state_mutex);
    enemies[i] = std::move(enemies[enemies.size() - 1]);
    enemies.pop_back();

    map[enemy->cur_pos.first][enemy->cur_pos.second].reset();
}

bool MapState::add_enemy(std::shared_ptr<Enemy> enemy) {
    std::scoped_lock lock(state_mutex);
    if (is_occupied(enemy->cur_pos)) {
        return false;
    }
    enemies.push_back(enemy);

    map[enemy->cur_pos.first][enemy->cur_pos.second] = enemy;
    return true;
}

bool MapState::run_level() {
    uint8_t direction = 0;

    // Load the current state of the game_running atomic_bool.
    while (game_running.load() && killed < DIFFICULTY) {
        direction = (direction + 1) % directions.size();
        if (enemies.size() < MAX_ENEMIES && spawned < DIFFICULTY) {
            add_enemy(std::make_shared<Goblin>(directions[direction]));
            spawned++;
        }
        std::size_t i = 0;
        while (i < enemies.size()) {
            auto enemy = enemies[i];
            if (enemy->cur_hp <= 0) {
                // Remove dead enemies
                remove_enemy(i, enemy);
                killed++;
                continue;
            }

            std::pair<Action, position> enemy_action;
            // Make the enemy act.
            enemy_action = enemy->decide_action(player->get_pos(state_mutex));
            switch (enemy_action.first) {
            case Action::Move:
                move_enemy(enemy_action.second, enemy);
                break;
            case Action::Attack:
                attack_pos(enemy_action.second, enemy->DAMAGE);
                break;
            }
            i++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    if (killed == DIFFICULTY) {
        return true;
    }
    return false;
}

// Check if the position is out of bounds. No lock is needed for this, since
// the bounds of the map never change.
bool MapState::is_out_of_bounds(position pos) const {
    return (pos.first < 0) || (pos.first >= ROWS) || (pos.second < 0) ||
           (pos.second >= COLUMNS);
}

// Check if the position is occupied. Every time we call this, we assume we're
// already holding the lock.
bool MapState::is_occupied(position pos) const {
    if (is_out_of_bounds(pos)) {
        return true;
    }

    return map[pos.first][pos.second] != nullptr;
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

void MapState::attack_pos(position pos, uint8_t dmg, uint8_t radius) {
    if (radius != 1) {
        return; // TODO: Add radius calculations
    }

    if (is_out_of_bounds(pos)) {
        return;
    }

    std::scoped_lock lock(state_mutex);
    auto& entity = map[pos.first][pos.second];
    if (entity == nullptr) {
        return;
    }
    auto remaining_hp = entity->take_damage(dmg);

    // end game if player is dead
    if (entity.get() == player.get() && remaining_hp <= 0) {
        game_running = false;
    }
}

void MapState::move_player(position goal_pos) {
    move_to_pos(player.get(), goal_pos);
}

void MapState::move_to_pos(Entity* entity, position new_pos) {
    position old_pos = entity->get_pos(state_mutex);

    {
        std::scoped_lock lock(state_mutex);

        if (is_occupied(new_pos)) {
            return;
        }

        std::swap(map[old_pos.first][old_pos.second],
                  map[new_pos.first][new_pos.second]);
    }

    entity->set_pos(state_mutex, new_pos);
}

void MapState::update_player_from_input(
    const InputState& input_state, std::chrono::steady_clock::time_point now) {
    // Player can only move at certain intervals.
    if (now - last_player_move_time_ < PLAYER_MOVE_INTERVAL) {
        return;
    }

    position pos_diff{0, 0};
    // Make the player move in the direction of the input, except if any keys
    // are held that would conflict (e.g. player is trying to move both up and
    // down at the same time).
    if (is_dir_active(input_state, InputDir::Up, false) &&
        !is_dir_active(input_state, InputDir::Down, false)) {
        pos_diff.first -= 1;
    } else if (is_dir_active(input_state, InputDir::Down, false) &&
               !is_dir_active(input_state, InputDir::Up, false)) {
        pos_diff.first += 1;
    }

    if (is_dir_active(input_state, InputDir::Left, false) &&
        !is_dir_active(input_state, InputDir::Right, false)) {
        pos_diff.second -= 1;
    } else if (is_dir_active(input_state, InputDir::Right, false) &&
               !is_dir_active(input_state, InputDir::Left, false)) {
        pos_diff.second += 1;
    }

    if (pos_diff.first == 0 && pos_diff.second == 0) {
        return;
    }

    last_player_move_time_ = now;
    const position cur_pos = player->get_pos(state_mutex);
    move_player({static_cast<int8_t>(cur_pos.first + pos_diff.first),
                 static_cast<int8_t>(cur_pos.second + pos_diff.second)});
}

void MapState::spawn_test_bullets(const InputState& input_state,
                                  std::chrono::steady_clock::time_point now) {
    const position player_pos = player->get_pos(state_mutex);

    // Create bullets for each held direction key (for input key testing).
    // TODO: Change this so that bullets can only be fired from one direction.
    // TODO: Add diagonal shooting?
    for (InputDir input_dir :
         {InputDir::Up, InputDir::Down, InputDir::Left, InputDir::Right}) {
        if (!is_dir_active(input_state, input_dir, true)) {
            continue;
        }

        const auto key_index = static_cast<size_t>(input_dir);
        // Make sure bullets are only fired at certain time intervals.
        if (now - last_bullet_fire_time_[key_index] < BULLET_FIRE_INTERVAL) {
            continue;
        }

        // Update the last fire time for this direction.
        last_bullet_fire_time_[key_index] = now;

        const position pos_diff = get_dir_diff(input_dir);
        const position spawn_pos{
            static_cast<int8_t>(player_pos.first + pos_diff.first),
            static_cast<int8_t>(player_pos.second + pos_diff.second)};
        // Check if spawn position is out of bounds.
        if (is_out_of_bounds(spawn_pos)) {
            continue;
        }

        // Get the lock, since we're changing shared data.
        std::scoped_lock lock(state_mutex);
        test_bullets.push_back({.pos = spawn_pos,
                                .pos_diff = pos_diff,
                                .icon = get_bullet_icon(input_dir),
                                .last_moved_at = now});
    }
}

// Moves all test bullets forward if enough time has passed since they last
// moved.
void MapState::move_test_bullets_forward(
    std::chrono::steady_clock::time_point now) {
    std::scoped_lock lock(state_mutex);

    // TODO: There's gotta be a more elegant solution to this...
    auto next_end = std::remove_if(
        test_bullets.begin(), test_bullets.end(),
        [this, now](TestBullet& bullet) {
            // Only move the bullet if enough time has passed.
            if (now - bullet.last_moved_at < BULLET_STEP_INTERVAL) {
                return false; // Don't remove the bullet.
            }

            bullet.last_moved_at = now;
            // Calculate the bullet's next position.
            const position next_pos{
                static_cast<int8_t>(bullet.pos.first + bullet.pos_diff.first),
                static_cast<int8_t>(bullet.pos.second +
                                    bullet.pos_diff.second)};
            // If the next position is out of bounds, remove the bullet.
            if (is_out_of_bounds(next_pos)) {
                return true; // Remove the bullet.
            }
            if (is_occupied(next_pos)) {
                auto& entity = map[next_pos.first][next_pos.second];
                if (entity == nullptr) {
                    return true;
                }
                entity->take_damage(player->DAMAGE);
            }

            bullet.pos = next_pos;
            return false; // Don't remove the bullet.
        });

    // Erase the bullets that need to be removed.
    test_bullets.erase(next_end, test_bullets.end());
}

// Creates the current rendering “state” or “snapshot” of the game, basically
// the data that render_frame() needs to render the current state of the game.
void MapState::create_render_state(
    EntityRenderData& player_render_data,
    std::vector<EntityRenderData>& enemies_render_data,
    std::vector<BulletRenderData>& bullets_render_data) const {
    // Create a lock for the duration of this function, since we're accessing
    // the shared state of the game.
    std::scoped_lock lock(state_mutex);

    player_render_data = {.pos = player->cur_pos, .icon = player->ICON};

    // Update the enemies' render data.
    enemies_render_data.clear();
    enemies_render_data.reserve(enemies.size());
    for (const auto& enemy : enemies) {
        enemies_render_data.push_back(
            {.pos = enemy->cur_pos, .icon = enemy->ICON});
    }

    // Update the bullets' render data.
    bullets_render_data.clear();
    bullets_render_data.reserve(test_bullets.size());
    for (const TestBullet& bullet : test_bullets) {
        bullets_render_data.push_back({.pos = bullet.pos, .icon = bullet.icon});
    }
}

position MapState::get_dir_diff(InputDir input_dir) {
    switch (input_dir) {
    case InputDir::Up:
        return {-1, 0};
    case InputDir::Down:
        return {1, 0};
    case InputDir::Left:
        return {0, -1};
    case InputDir::Right:
        return {0, 1};
    }

    return {0, 0};
}

char MapState::get_bullet_icon(InputDir input_dir) {
    switch (input_dir) {
    case InputDir::Up:
        return '^';
    case InputDir::Down:
        return 'v';
    case InputDir::Left:
        return '<';
    case InputDir::Right:
        return '>';
    }

    return '*';
}

void MapState::ncurses_thread() {
    while (game_running.load()) {
        const InputState input_state = SCREEN.consume_input_state();
        if (input_state.quit_requested) {
            game_running = false;
            break;
        }

        // Create a steady_clock time point for the current time, that will be
        // used for the upcoming methods.
        const auto now = std::chrono::steady_clock::now();
        update_player_from_input(input_state, now);
        spawn_test_bullets(input_state, now);
        move_test_bullets_forward(now);

        // Create render data for render_frame().
        // TODO: Replace the whole “render data” system with just getting the
        // data from the entities directly.
        EntityRenderData player_render;
        std::vector<EntityRenderData> enemies_render_data;
        std::vector<BulletRenderData> bullets_render_data;
        create_render_state(player_render, enemies_render_data,
                            bullets_render_data);

        SCREEN.render_frame(player_render, enemies_render_data,
                            bullets_render_data, ROWS, COLUMNS, input_state);
        SCREEN.sleep_until_next_frame();
    }
}
