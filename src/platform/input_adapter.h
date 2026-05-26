#pragma once

#include <SDL3/SDL.h>

#include "../input.h"
#include "../ui/focus/ui_focus.h"

namespace platform {

enum class GamepadButton {
    Up,
    Down,
    Left,
    Right,
    Confirm,
    Cancel,
};

bool keycode_from_name(const char *name, SDL_Keycode *out);
bool gamepad_button_from_name(const char *name, GamepadButton *out);

void apply_key_down(SDL_Keycode key,
                    InputState &demo_input,
                    ::ui::UiInputFrame &ui_input,
                    bool *running);

void apply_key_up(SDL_Keycode key, ::ui::UiInputFrame &ui_input);
void apply_gamepad_button_down(GamepadButton button, ::ui::UiInputFrame &ui_input);
void apply_gamepad_button_up(GamepadButton button, ::ui::UiInputFrame &ui_input);

} // namespace platform
