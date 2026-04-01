#pragma once

#include "entities.h"
#include <array>
#include <cstdint>
#include <notcurses/notcurses.h>
#include <vector>

// The key input direction for both moving and firing.
enum class InputDir : uint8_t { Up = 0, Down, Left, Right };
constexpr uint8_t INPUT_DIRECTIONS_COUNT = 4;

// TODO: Replace the whole “render data” system with just getting the data
// from the entities directly.
// ↓Temporary, just for testing↓.
struct BulletRenderData {
    position pos{};
    char icon = '*';
};

struct EntityRenderData {
    position pos{};
    char icon = '?';
};
// ↑Temporary, just for testing↑.

struct InputState {
    bool quit_requested = false;
    bool kitty_protocol_active = false;
    std::array<bool, ::INPUT_DIRECTIONS_COUNT> move_key_held{};
    std::array<bool, ::INPUT_DIRECTIONS_COUNT> fire_key_held{};
};

class NcursesScreen final {
  public:
    explicit NcursesScreen(uint8_t frames_per_second);
    ~NcursesScreen();

    NcursesScreen(const NcursesScreen&) = delete;
    NcursesScreen& operator=(const NcursesScreen&) = delete;

    InputState consume_input_state();
    void render_frame(EntityRenderData player,
                      const std::vector<EntityRenderData>& enemies_render_data,
                      const std::vector<BulletRenderData>& bullets_render_data,
                      int8_t rows, int8_t columns,
                      const InputState& input_state);
    void sleep_until_next_frame() const;

  private:
    void setup_battle_plane(int8_t rows, int8_t columns);
    void handle_input_event(uint32_t event_id, const ncinput& input_event);
    void set_direction_state(InputDir input_dir, bool is_fire,
                             ncintype_e event_type);

    uint8_t refresh_interval_ms_;
    notcurses* notcurses_ = nullptr;
    ncplane* battle_plane_ = nullptr;
    int8_t battle_rows_ = -1;
    int8_t battle_columns_ = -1;
    InputState input_state_;
    bool kitty_terminal_detected_ = false;
};
