#include <cstdint>
#include <utility>

void melee_attack();
void ranged_attack();

class Entity {
  public:
    const int8_t max_hp;
    const int8_t damage;
    const int8_t move_speed;
    int8_t cur_hp;
    std::pair<int8_t, int8_t> cur_pos;

    virtual ~Entity() = default;

  protected:
    void move();

    virtual void attack() = 0;
    virtual int8_t take_damage(int8_t dmg) = 0;
    virtual void update() = 0;

    Entity(int8_t max_health, int8_t dmg, int8_t move_speed, int8_t x, int8_t y)
        : max_hp(max_health), damage(dmg), move_speed(move_speed),
          cur_hp(max_hp), cur_pos(x, y) {};
};
