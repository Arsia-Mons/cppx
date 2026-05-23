#pragma once

#include "ui/runtime/UiFrameContext.h"

#include <clay.h>

#include <functional>
#include <string>

namespace ui {

void Field(
    UiFrameContext &frame,
    Clay_ElementId id,
    const std::string &label,
    const std::string &value,
    bool focused,
    bool password,
    std::function<void()> onFocus
);

}
