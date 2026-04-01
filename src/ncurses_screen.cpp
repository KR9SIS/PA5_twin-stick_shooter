#include "ncurses_screen.h"
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <thread>

namespace {
constexpr int8_t BATTLE_PLANE_START_ROW = 4;
constexpr int8_t BATTLE_PLANE_START_COLUMN = 0;
// #00FFFF
constexpr uint8_t PLAYER_RGB_R = 0;
constexpr uint8_t PLAYER_RGB_G = 255;
constexpr uint8_t PLAYER_RGB_B = 255;
// #FF0000
constexpr uint8_t ENEMY_RGB_R = 255;
constexpr uint8_t ENEMY_RGB_G = 0;
constexpr uint8_t ENEMY_RGB_B = 0;
// #FFFF00
constexpr uint8_t BULLET_RGB_R = 255;
constexpr uint8_t BULLET_RGB_G = 255;
constexpr uint8_t BULLET_RGB_B = 0;
// #FFFFFF
constexpr uint8_t BORDER_RGB_R = 255;
constexpr uint8_t BORDER_RGB_G = 255;
constexpr uint8_t BORDER_RGB_B = 255;

// Converts an InputDir to its corresponding index in the input state arrays.
size_t to_dir_index(InputDir input_dir) {
    return static_cast<size_t>(input_dir);
}

// Gets the character to display for a key based on whether it's held or not.
char held_symbol(bool held) {
    if (held) {
        return 'h';
    }
    return '.';
}

// Converts a Notcurses event ID to a lowercase ASCII character, if possible.
char to_lower_ascii_char(uint32_t input_event_id) {
    if (input_event_id > 0x7f) {
        return '\0';
    }
    return static_cast<char>(
        std::tolower(static_cast<unsigned char>(input_event_id)));
}

// Sets the foreground & background colors of the given plane to the default
// terminal colors.
void set_default_colors(ncplane* plane) {
    ::ncplane_set_fg_default(plane);
    ::ncplane_set_bg_default(plane);
}

// Updates the foreground color of the given plane to the given RGB values, and
// keep the background color as the default terminal background.
void update_plane_rgb(ncplane* plane, uint8_t red, uint8_t green,
                      uint8_t blue) {
    ::ncplane_set_fg_rgb8(plane, red, green, blue);
    ::ncplane_set_bg_default(plane);
}

// Creates an ncchannels value with the given RGB foreground color and the
// default background color.
uint64_t create_rgb_ncchannels(uint8_t red, uint8_t green, uint8_t blue) {
    uint64_t rgb_channels = 0;
    ::ncchannels_set_fg_rgb8(&rgb_channels, red, green, blue);
    ::ncchannels_set_bg_default(&rgb_channels);
    return rgb_channels;
}
} // namespace

NcursesScreen::NcursesScreen(uint8_t frames_per_second)
    : refresh_interval_ms_(static_cast<uint8_t>(1000 / frames_per_second)) {
    notcurses_options options{}; // Create Notcurses default options.
    options.flags = NCOPTION_SUPPRESS_BANNERS; // Suppress startup banner.
    // Initialize Notcurses with our options.
    notcurses_ = ::notcurses_core_init(&options, stdout);
    // Check if Notcurses initialization succeeded.
    if (notcurses_ == nullptr) {
        throw std::runtime_error("Failed to initialize Notcurses.");
    }

    // Detect if the game is running in a Kitty terminal from the environment
    // variables that Kitty sets.
    kitty_terminal_detected_ = std::getenv("KITTY_WINDOW_ID") != nullptr ||
                               std::getenv("KITTY_PID") != nullptr;
    input_state_.kitty_protocol_active = kitty_terminal_detected_;

    (void)::notcurses_cursor_disable(notcurses_); // Disable the cursor.
}

NcursesScreen::~NcursesScreen() {
    // Destroy the battle plane if it exists.
    if (battle_plane_ != nullptr) {
        ::ncplane_destroy(battle_plane_);
        battle_plane_ = nullptr;
    }

    // Stop Notcurses if it's still running.
    if (notcurses_ != nullptr) {
        (void)::notcurses_stop(notcurses_);
        notcurses_ = nullptr;
    }
}

InputState NcursesScreen::consume_input_state() {
    // If the Kitty keyboard protocol isn't active, we can assume that no keys
    // are currently held, since without the protocol we only get events on key
    // presses, not on releases.
    if (!input_state_.kitty_protocol_active) {
        input_state_.move_key_held.fill(false);
        input_state_.fire_key_held.fill(false);
    }

    ncinput input_event{}; // Input event data.
    // Event ID of the current input event.
    uint32_t input_event_id = ::notcurses_get_nblock(notcurses_, &input_event);
    while (input_event_id != 0 && input_event_id != static_cast<uint32_t>(-1)) {
        handle_input_event(input_event_id, input_event); // Handle it.
        // Get the event ID of the next input event.
        input_event_id = ::notcurses_get_nblock(notcurses_, &input_event);
    }

    // Event ID “-1” means the user requested to quit (e.g. by pressing
    // Ctrl+C).
    if (input_event_id == static_cast<uint32_t>(-1)) {
        input_state_.quit_requested = true;
    }

    return input_state_;
}

void NcursesScreen::render_frame(
    EntityRenderData player,
    const std::vector<EntityRenderData>& enemies_render_data,
    const std::vector<BulletRenderData>& bullets_render_data, int8_t rows,
    int8_t columns, const InputState& input_state) {
    // Create the standard plane, i.e. the entire terminal window.
    ncplane* const stdplane = ::notcurses_stdplane(notcurses_);
    ::ncplane_erase(stdplane);
    ::set_default_colors(stdplane);

    // Print instructions at the top of the screen.
    ::ncplane_putstr_yx(
        stdplane, 0, 0,
        "Hold wasd to move, arrow keys to fire bullets, q to quit.");
    ::ncplane_printf_yx(stdplane, 1, 0, "Keyboard mode: %s",
                        input_state.kitty_protocol_active
                            ? "Notcurses + Kitty"
                            : "Notcurses fallback");
    // Print the state of the movement and firing keys.
    ::ncplane_printf_yx(
        stdplane, 2, 0,
        "Move:W[%c] A[%c] S[%c] D[%c] | Fire:^[%c] <[%c] v[%c] >[%c]",
        held_symbol(input_state.move_key_held[to_dir_index(InputDir::Up)]),
        held_symbol(input_state.move_key_held[to_dir_index(InputDir::Left)]),
        held_symbol(input_state.move_key_held[to_dir_index(InputDir::Down)]),
        held_symbol(input_state.move_key_held[to_dir_index(InputDir::Right)]),
        held_symbol(input_state.fire_key_held[to_dir_index(InputDir::Up)]),
        held_symbol(input_state.fire_key_held[to_dir_index(InputDir::Left)]),
        held_symbol(input_state.fire_key_held[to_dir_index(InputDir::Down)]),
        held_symbol(input_state.fire_key_held[to_dir_index(InputDir::Right)]));

    // Set up our battle plane.
    setup_battle_plane(rows, columns);
    ::ncplane_erase(battle_plane_);
    set_default_colors(battle_plane_);
    (void)::ncplane_perimeter_double(
        battle_plane_, 0,
        create_rgb_ncchannels(BORDER_RGB_R, BORDER_RGB_G, BORDER_RGB_B), 0);

    // Draw the player.
    update_plane_rgb(battle_plane_, PLAYER_RGB_R, PLAYER_RGB_G, PLAYER_RGB_B);
    ::ncplane_putchar_yx(battle_plane_, player.pos.first + 1,
                         player.pos.second + 1, player.icon);

    // Draw the enemies.
    update_plane_rgb(battle_plane_, ENEMY_RGB_R, ENEMY_RGB_G, ENEMY_RGB_B);
    for (const EntityRenderData& enemy : enemies_render_data) {
        ::ncplane_putchar_yx(battle_plane_, enemy.pos.first + 1,
                             enemy.pos.second + 1, enemy.icon);
    }

    // Draw the bullets.
    update_plane_rgb(battle_plane_, BULLET_RGB_R, BULLET_RGB_G, BULLET_RGB_B);
    for (const BulletRenderData& bullet : bullets_render_data) {
        ::ncplane_putchar_yx(battle_plane_, bullet.pos.first + 1,
                             bullet.pos.second + 1, bullet.icon);
    }

    (void)::notcurses_render(notcurses_); // Render everything.
}

void NcursesScreen::sleep_until_next_frame() const {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(refresh_interval_ms_));
}

void NcursesScreen::setup_battle_plane(int8_t rows, int8_t columns) {
    if (battle_plane_ == nullptr) {
        ncplane_options options{
            .y = BATTLE_PLANE_START_ROW,
            .x = BATTLE_PLANE_START_COLUMN,
            .rows = static_cast<unsigned>(rows + 2),
            .cols = static_cast<unsigned>(columns + 2),
            .userptr = nullptr,
            .name = "battle-plane",
            .resizecb = nullptr,
            .flags = 0,
            .margin_b = 0,
            .margin_r = 0,
        };
        // Create the battle plane as a subwindow of the standard plane.
        battle_plane_ =
            ::ncplane_create(::notcurses_stdplane(notcurses_), &options);
        // Check if it was created successfully.
        if (battle_plane_ == nullptr) {
            throw std::runtime_error("Failed to create battle plane.");
        }
        battle_rows_ = rows;
        battle_columns_ = columns;
        return;
    }

    // If the battle plane already exists, and is the correct size, do nothing.
    if (battle_rows_ == rows && battle_columns_ == columns) {
        return;
    }

    // Handle the case where the battle plane already exists, but was resized.
    (void)::ncplane_move_yx(battle_plane_, BATTLE_PLANE_START_ROW,
                            BATTLE_PLANE_START_COLUMN);
    (void)::ncplane_resize_simple(battle_plane_, rows + 2, columns + 2);
    battle_rows_ = rows;
    battle_columns_ = columns;
}

// Handles a Notcurses input event by updating the corresponding input state.
void NcursesScreen::handle_input_event(uint32_t input_event_id,
                                       const ncinput& input_event) {
    // Ignore resize events.
    if (input_event_id == NCKEY_RESIZE) {
        return;
    }

    // Signs that the Kitty keyboard protocol is 100% active.
    if (input_event.evtype == NCTYPE_REPEAT ||
        input_event.evtype == NCTYPE_RELEASE) {
        input_state_.kitty_protocol_active = true;
    }

    // Arrow keys for firing.
    switch (input_event_id) {
    case NCKEY_UP:
        set_direction_state(InputDir::Up, true, input_event.evtype);
        return;
    case NCKEY_DOWN:
        set_direction_state(InputDir::Down, true, input_event.evtype);
        return;
    case NCKEY_LEFT:
        set_direction_state(InputDir::Left, true, input_event.evtype);
        return;
    case NCKEY_RIGHT:
        set_direction_state(InputDir::Right, true, input_event.evtype);
        return;
    default:
        break;
    }

    // WASD keys for moving, q for quitting.
    switch (to_lower_ascii_char(input_event_id)) {
    case 'w':
        set_direction_state(InputDir::Up, false, input_event.evtype);
        return;
    case 's':
        set_direction_state(InputDir::Down, false, input_event.evtype);
        return;
    case 'a':
        set_direction_state(InputDir::Left, false, input_event.evtype);
        return;
    case 'd':
        set_direction_state(InputDir::Right, false, input_event.evtype);
        return;
    case 'q':
        input_state_.quit_requested = true;
        return;
    default:
        return;
    }
}

// Updates the input state for the provided key input direction based on
// whether it's a firing key or movement key, and whether the event is a key
// press or key release.
void NcursesScreen::set_direction_state(InputDir input_dir, bool firing,
                                        ncintype_e event_type) {
    const size_t key_dir_index = to_dir_index(input_dir);
    // Get the appropriate array based on whether a firing key or movement key
    // was pressed/released.
    auto& key_type =
        firing ? input_state_.fire_key_held : input_state_.move_key_held;
    // Update the held state of the key.
    if (event_type == NCTYPE_RELEASE) {
        key_type[key_dir_index] = false;
        return;
    } else {
        key_type[key_dir_index] = true;
    }
}
