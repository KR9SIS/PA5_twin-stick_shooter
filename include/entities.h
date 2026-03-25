#include <cstdint>
#include <utility>

void melee_attack();
void ranged_attack();

enum class Direction { UP, DOWN, LEFT, RIGHT };
enum class Action { Attack, Move };

class Entity {
  public:
    const int8_t max_hp;
    const int8_t damage;
    const int8_t move_speed;
    int8_t cur_hp;
    std::pair<uint8_t, uint8_t> cur_pos;

    virtual ~Entity() = default;

  protected:
    void move(Direction d);

    virtual void attack() = 0;
    virtual int8_t take_damage(int8_t dmg) = 0;
    virtual void update() = 0;

    Entity(int8_t max_health, int8_t dmg, int8_t move_speed, int8_t start_x,
           int8_t start_y)
        : max_hp(max_health), damage(dmg), move_speed(move_speed),
          cur_hp(max_hp), cur_pos(start_x, start_y) {};
};

class Player : Entity {
  public:
    void attack();
    int8_t take_damage(int8_t dmg);
    void update();

    Player();
};

class Enemy : Entity {
  public:
    void act();
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
