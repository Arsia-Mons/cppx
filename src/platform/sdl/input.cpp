#include "input.h"

#include <string.h>

namespace platform {

static bool equals_key_name(const char *a, const char *b) {
  return a && b && SDL_strcasecmp(a, b) == 0;
}

bool keycode_from_name(const char *name, SDL_Keycode *out) {
  if (!name || !out)
    return false;
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
  if (equals_key_name(name, "backspace")) {
    *out = SDLK_BACKSPACE;
    return true;
  }
  if (equals_key_name(name, "delete") || equals_key_name(name, "del")) {
    *out = SDLK_DELETE;
    return true;
  }
  if (equals_key_name(name, "home")) {
    *out = SDLK_HOME;
    return true;
  }
  if (equals_key_name(name, "end")) {
    *out = SDLK_END;
    return true;
  }
  if (equals_key_name(name, "tab")) {
    *out = SDLK_TAB;
    return true;
  }
  if (equals_key_name(name, "a")) {
    *out = SDLK_A;
    return true;
  }
  return false;
}

static uint16_t ui_modifiers(SDL_Keymod mod) {
  uint16_t out = ::ui::UI_KEY_MOD_NONE;
  if ((mod & SDL_KMOD_SHIFT) != 0)
    out |= ::ui::UI_KEY_MOD_SHIFT;
  if ((mod & SDL_KMOD_CTRL) != 0)
    out |= ::ui::UI_KEY_MOD_CTRL;
  if ((mod & SDL_KMOD_ALT) != 0)
    out |= ::ui::UI_KEY_MOD_ALT;
  if ((mod & SDL_KMOD_GUI) != 0)
    out |= ::ui::UI_KEY_MOD_SUPER;
  return out;
}

static ::ui::UiKey ui_key_from_sdl(SDL_Keycode key) {
  switch (key) {
  case SDLK_BACKSPACE:
    return ::ui::UiKey::Backspace;
  case SDLK_DELETE:
    return ::ui::UiKey::DeleteForward;
  case SDLK_LEFT:
    return ::ui::UiKey::Left;
  case SDLK_RIGHT:
    return ::ui::UiKey::Right;
  case SDLK_HOME:
    return ::ui::UiKey::Home;
  case SDLK_END:
    return ::ui::UiKey::End;
  case SDLK_RETURN:
    return ::ui::UiKey::Enter;
  case SDLK_TAB:
    return ::ui::UiKey::Tab;
  case SDLK_A:
    return ::ui::UiKey::A;
  default:
    return ::ui::UiKey::Unknown;
  }
}

bool gamepad_button_from_name(const char *name, GamepadButton *out) {
  if (!name || !out)
    return false;
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
  if (equals_key_name(name, "a") || equals_key_name(name, "south") ||
      equals_key_name(name, "confirm")) {
    *out = GamepadButton::Confirm;
    return true;
  }
  if (equals_key_name(name, "b") || equals_key_name(name, "east") ||
      equals_key_name(name, "cancel")) {
    *out = GamepadButton::Cancel;
    return true;
  }
  return false;
}

void apply_key_down(SDL_Keycode key, ::ui::UiInputFrame &ui_input,
                    bool *running) {
  ::ui::UiKey ui_key = ui_key_from_sdl(key);
  if (ui_key != ::ui::UiKey::Unknown) {
    ::ui::ui_input_push_key(ui_input, ui_key, ui_modifiers(SDL_GetModState()));
  }

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

void apply_text_input(const char *text, ::ui::UiInputFrame &ui_input) {
  ::ui::ui_input_push_text(ui_input, text);
  ui_input.source = ::ui::UiFocusSource::Keyboard;
}

void apply_text_editing(const char *text, int start, int length,
                        ::ui::UiInputFrame &ui_input) {
  ::ui::ui_input_push_editing(ui_input, text, start, length);
  ui_input.source = ::ui::UiFocusSource::Keyboard;
}

void apply_gamepad_button_down(GamepadButton button,
                               ::ui::UiInputFrame &ui_input) {
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

void apply_gamepad_button_up(GamepadButton button,
                             ::ui::UiInputFrame &ui_input) {
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
