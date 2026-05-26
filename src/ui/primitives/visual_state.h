#pragma once

#include "../focus/ui_focus.h"

namespace ui {

struct ControlState {
    bool checked = false;
    bool selected = false;
    bool disabled = false;
};

struct VisualState {
    bool targeted = false;
    bool active = false;
    bool chosen = false;
    bool unavailable = false;
};

inline VisualState derive_visual_state(const UiFocusableState &focus,
                                       const ControlState &control) {
    return {
        .targeted = focus.hovered || (focus.focused && focus.focus_visible),
        .active = focus.pressed,
        .chosen = control.checked || control.selected,
        .unavailable = control.disabled,
    };
}

} // namespace ui
