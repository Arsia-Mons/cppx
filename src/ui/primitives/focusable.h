#pragma once

#include <functional>

#include "../focus/ui_focus.h"

namespace ui {

using FocusableRender = std::function<void(const UiFocusableState &focus)>;

struct FocusableProps {
    Clay_ElementId id = {};
    bool disabled = false;
    UiNavRules nav = {};
    std::function<void()> on_confirm = {};
    std::function<void()> on_focus = {};
};

void Focusable(const FocusableProps &props, FocusableRender render);

} // namespace ui
