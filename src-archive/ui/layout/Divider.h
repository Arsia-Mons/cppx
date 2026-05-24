#pragma once

#include "ui/design/Theme.h"
#include "ui/layout/Box.h"

namespace ui {

inline void Divider(UiFrameContext &frame, Clay_ElementId id) {
    Box(frame, {
        .id = id,
        .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(1)}},
        .backgroundColor = color::Border
    }, [] {});
}

}
