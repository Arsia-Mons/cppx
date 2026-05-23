#pragma once

#include "ui/runtime/UiFrameContext.h"

#include <clay.h>

#include <functional>
#include <string>

namespace ui {

enum class ButtonVariant {
    Primary,
    Secondary,
    Ghost,
    Danger,
};

void Button(
    UiFrameContext &frame,
    Clay_ElementId id,
    const std::string &label,
    ButtonVariant variant,
    std::function<void()> onPressed
);

}
