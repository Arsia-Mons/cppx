#pragma once

#include "ui/layout/Box.h"

namespace ui {

inline void Spacer(UiFrameContext &frame, Clay_ElementId id, Clay_Sizing sizing) {
    Box(frame, {
        .id = id,
        .layout = {.sizing = sizing}
    }, [] {});
}

}
