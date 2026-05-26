#include "input_adapter.h"

#include <string.h>

namespace platform {

static bool equals_key_name(const char *a, const char *b) {
    return a && b && SDL_strcasecmp(a, b) == 0;
}

bool keycode_from_name(const char *name, SDL_Keycode *out) {
    if (!name || !out) return false;
    if (equals_key_name(name, "escape") || equals_key_name(name, "esc")) {
        *out = SDLK_ESCAPE;
        return true;
    }
    if (equals_key_name(name, "up")) {
        *out = SDLK_UP;
        return true;
    }
    if (equals_key_name(name, "down")) {
        *out = SDLK_DOWN;
        return true;
    }
    if (equals_key_name(name, "left")) {
        *out = SDLK_LEFT;
        return true;
    }
    if (equals_key_name(name, "right")) {
        *out = SDLK_RIGHT;
        return true;
    }
    if (equals_key_name(name, "return") || equals_key_name(name, "enter")) {
        *out = SDLK_RETURN;
        return true;
    }
    if (equals_key_name(name, "space")) {
        *out = SDLK_SPACE;
        return true;
    }
    if (equals_key_name(name, "m")) {
        *out = SDLK_M;
        return true;
    }
    if (equals_key_name(name, "t")) {
        *out = SDLK_T;
        return true;
    }
    if (equals_key_name(name, "i")) {
        *out = SDLK_I;
        return true;
    }
    return false;
}

void apply_key_down(SDL_Keycode key,
                    InputState &demo_input,
                    ::ui::UiInputFrame &ui_input,
                    bool *running) {
    switch (key) {
        case SDLK_ESCAPE:
            ui_input.cancel_pressed = true;
            ui_input.cancel_down = true;
            ui_input.source = ::ui::UiFocusSource::Keyboard;
            if (running) *running = false;
            break;
        case SDLK_UP:
            demo_input.increment_counter = true;
            ui_input.nav_up = true;
            ui_input.source = ::ui::UiFocusSource::Keyboard;
            break;
        case SDLK_DOWN:
            demo_input.decrement_counter = true;
            ui_input.nav_down = true;
            ui_input.source = ::ui::UiFocusSource::Keyboard;
            break;
        case SDLK_LEFT:
            ui_input.nav_left = true;
            ui_input.source = ::ui::UiFocusSource::Keyboard;
            break;
        case SDLK_RIGHT:
            ui_input.nav_right = true;
            ui_input.source = ::ui::UiFocusSource::Keyboard;
            break;
        case SDLK_RETURN:
        case SDLK_SPACE:
            ui_input.confirm_pressed = true;
            ui_input.confirm_down = true;
            ui_input.source = ::ui::UiFocusSource::Keyboard;
            break;
        case SDLK_M:
            demo_input.toggle_counter = true;
            break;
        case SDLK_T:
            demo_input.cycle_theme = true;
            break;
        case SDLK_I:
            demo_input.fetch_image = true;
            break;
        default:
            break;
    }
}

void apply_key_up(SDL_Keycode key, ::ui::UiInputFrame &ui_input) {
    switch (key) {
        case SDLK_ESCAPE:
            ui_input.cancel_released = true;
            ui_input.source = ::ui::UiFocusSource::Keyboard;
            break;
        case SDLK_RETURN:
        case SDLK_SPACE:
            ui_input.confirm_released = true;
            ui_input.source = ::ui::UiFocusSource::Keyboard;
            break;
        default:
            break;
    }
}

} // namespace platform
