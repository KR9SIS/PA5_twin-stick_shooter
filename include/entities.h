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

    std::pair<uint8_t, uint8_t> get_pos();
    void set_pos(std::pair<uint8_t, uint8_t> new_pos);

  protected:
    void move(Direction d);

    virtual void attack() = 0;
    virtual int8_t take_damage(int8_t dmg) = 0;
    virtual void update() = 0;

    Entity(int8_t max_health, int8_t dmg, int8_t move_speed, uint8_t start_x,
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
    std::pair<Action, std::pair<uint8_t, uint8_t>>
    act(std::pair<uint8_t, uint8_t> player_pos);
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
