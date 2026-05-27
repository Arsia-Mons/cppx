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
    return false;
}

bool gamepad_button_from_name(const char *name, GamepadButton *out) {
    if (!name || !out) return false;
    if (equals_key_name(name, "up") || equals_key_name(name, "dpad_up")) {
        *out = GamepadButton::Up;
        return true;
    }
    if (equals_key_name(name, "down") || equals_key_name(name, "dpad_down")) {
        *out = GamepadButton::Down;
        return true;
    }
    if (equals_key_name(name, "left") || equals_key_name(name, "dpad_left")) {
        *out = GamepadButton::Left;
        return true;
    }
    if (equals_key_name(name, "right") || equals_key_name(name, "dpad_right")) {
        *out = GamepadButton::Right;
        return true;
    }
    if (equals_key_name(name, "a") ||
        equals_key_name(name, "south") ||
        equals_key_name(name, "confirm")) {
        *out = GamepadButton::Confirm;
        return true;
    }
    if (equals_key_name(name, "b") ||
        equals_key_name(name, "east") ||
        equals_key_name(name, "cancel")) {
        *out = GamepadButton::Cancel;
        return true;
    }
    return false;
}

void apply_key_down(SDL_Keycode key, ::ui::UiInputFrame &ui_input, bool *running) {
    switch (key) {
        case SDLK_ESCAPE:
            ui_input.cancel_pressed = true;
            ui_input.cancel_down = true;
            ui_input.source = ::ui::UiFocusSource::Keyboard;
            break;
        case SDLK_UP:
            ui_input.nav_up = true;
            ui_input.source = ::ui::UiFocusSource::Keyboard;
            break;
        case SDLK_DOWN:
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

void apply_gamepad_button_down(GamepadButton button, ::ui::UiInputFrame &ui_input) {
    ui_input.source = ::ui::UiFocusSource::Gamepad;
    switch (button) {
        case GamepadButton::Up:
            ui_input.nav_up = true;
            break;
        case GamepadButton::Down:
            ui_input.nav_down = true;
            break;
        case GamepadButton::Left:
            ui_input.nav_left = true;
            break;
        case GamepadButton::Right:
            ui_input.nav_right = true;
            break;
        case GamepadButton::Confirm:
            ui_input.confirm_pressed = true;
            ui_input.confirm_down = true;
            break;
        case GamepadButton::Cancel:
            ui_input.cancel_pressed = true;
            ui_input.cancel_down = true;
            break;
    }
}

void apply_gamepad_button_up(GamepadButton button, ::ui::UiInputFrame &ui_input) {
    ui_input.source = ::ui::UiFocusSource::Gamepad;
    switch (button) {
        case GamepadButton::Confirm:
            ui_input.confirm_released = true;
            break;
        case GamepadButton::Cancel:
            ui_input.cancel_released = true;
            break;
        case GamepadButton::Up:
        case GamepadButton::Down:
        case GamepadButton::Left:
        case GamepadButton::Right:
            break;
    }
}

} // namespace platform
