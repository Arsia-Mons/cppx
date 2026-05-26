#pragma once

#include <functional>

#include "../focus/ui_focus.h"

namespace ui {

struct ToggleProps {
    Clay_ElementId id = {};
    const char *label = "";
    bool checked = false;
    bool disabled = false;
    UiNavRules nav = {};
    std::function<void(bool)> on_change = {};
};

void Toggle(const ToggleProps &props);

} // namespace ui
