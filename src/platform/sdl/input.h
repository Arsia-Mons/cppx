#pragma once

#include <SDL3/SDL.h>

#include "../../ui/input.h"

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

void apply_key_down(SDL_Keycode key, ::ui::UiInputFrame &ui_input,
                    bool *running);

void apply_key_up(SDL_Keycode key, ::ui::UiInputFrame &ui_input);
void apply_text_input(const char *text, ::ui::UiInputFrame &ui_input);
void apply_text_editing(const char *text, int start, int length,
                        ::ui::UiInputFrame &ui_input);
void apply_gamepad_button_down(GamepadButton button,
                               ::ui::UiInputFrame &ui_input);
void apply_gamepad_button_up(GamepadButton button,
                             ::ui::UiInputFrame &ui_input);

} // namespace platform
