#include <cstdint>
#include <utility>

void melee_attack();
void ranged_attack();

enum class Direction { UP, DOWN, LEFT, RIGHT };
enum class Action { Attack, Move };

using position = std::pair<int8_t, int8_t>;

class Entity {
  public:
    const uint8_t max_hp;
    const uint8_t damage;
    const uint8_t move_speed;
    int8_t cur_hp;
    position cur_pos;

    virtual ~Entity() = default;

    position get_pos();
    void set_pos(position new_pos);
    void change_health(int8_t dmg);

  protected:
    Entity(uint8_t max_health, uint8_t dmg, uint8_t move_speed, uint8_t start_x,
           uint8_t start_y)
        : max_hp(max_health), damage(dmg), move_speed(move_speed),
          cur_hp(max_hp), cur_pos(start_y, start_x) {};
};

class Player : public Entity {
  public:
    void attack();
    int8_t take_damage(int8_t dmg);
    void update();

    Player();
};

class Enemy : public Entity {
  public:
    std::pair<Action, position> act(position player_pos);
};

class Giant : Enemy {
  public:
    void attack() {
        void melee_attack();
    }
};

class Goblin : Enemy {
  public:
    void attack() {
        void melee_attack();
    }
};

class Wizard : Enemy {
  public:
    void attack() {
        void ranged_attack();
    }
};
