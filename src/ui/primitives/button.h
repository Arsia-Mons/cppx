#pragma once

#include <functional>

#include "../focus/ui_focus.h"

namespace ui {

struct ButtonProps {
    Clay_ElementId id = {};
    const char *label = "";
    bool disabled = false;
    UiNavRules nav = {};
    std::function<void()> on_confirm = {};
};

void Button(const ButtonProps &props);

} // namespace ui
