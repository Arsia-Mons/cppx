#pragma once

#include <functional>

#include "../focus/ui_focus.h"

namespace ui {

struct SelectableProps {
    Clay_ElementId id = {};
    const char *label = "";
    bool selected = false;
    bool disabled = false;
    UiNavRules nav = {};
    std::function<void()> on_select = {};
};

void Selectable(const SelectableProps &props);

} // namespace ui
