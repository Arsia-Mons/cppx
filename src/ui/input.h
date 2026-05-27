#pragma once

namespace ui {

enum class UiFocusSource {
    None,
    Keyboard,
    Gamepad,
    Mouse,
    Touch,
    Programmatic,
};

struct UiInputFrame {
    bool nav_up = false;
    bool nav_down = false;
    bool nav_left = false;
    bool nav_right = false;

    bool confirm_pressed = false;
    bool confirm_down = false;
    bool confirm_released = false;

    bool cancel_pressed = false;
    bool cancel_down = false;
    bool cancel_released = false;

    bool pointer_pressed = false;
    bool pointer_down = false;
    bool pointer_released = false;

    UiFocusSource source = UiFocusSource::Keyboard;
};

} // namespace ui
