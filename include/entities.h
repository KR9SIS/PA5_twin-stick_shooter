#include <cstdint>
#include <mutex>
#include <utility>

#pragma once

void melee_attack();
void ranged_attack();

enum class Direction { UP, DOWN, LEFT, RIGHT };
enum class Action { Attack, Move };

using position = std::pair<int8_t, int8_t>;

class Entity {
  public:
    const uint8_t MAX_HP;
    const uint8_t DAMAGE;
    const uint8_t MOVE_SPEED;
    const char ICON;
    int8_t cur_hp;
    position cur_pos;

    virtual ~Entity() = default;

    position get_pos(std::mutex& state_mutex) const;
    void set_pos(std::mutex& state_mutex, position new_pos);
    void change_health(int8_t dmg);

    int8_t take_damage(int8_t dmg);

  protected:
    Entity(uint8_t max_health, uint8_t dmg, uint8_t move_speed, const char icon,
           uint8_t start_y, uint8_t start_x)
        : MAX_HP(max_health), DAMAGE(dmg), MOVE_SPEED(move_speed), ICON(icon),
          cur_hp(MAX_HP), cur_pos(start_y, start_x) {};
};

class Player : public Entity {
  public:
    void attack();
    void update();

    Player(uint8_t start_y, uint8_t start_x)
        : Entity(10, 2, UINT8_MAX, '@', start_y, start_x) {}
};

class Enemy : public Entity {
  public:
    std::pair<Action, position> decide_action(position player_pos);
    using Entity::Entity;
};

class Giant : public Enemy {
  public:
    void attack() {
        void melee_attack();
    }
};

class Goblin : public Enemy {
  public:
    void attack() {
        void melee_attack();
    }
    Goblin(position start_pos)
        : Enemy(5, 1, 3, 'g', start_pos.first, start_pos.second) {}
};

class Wizard : public Enemy {
  public:
    void attack() {
        void ranged_attack();
    }
};
