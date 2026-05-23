#pragma once

#include "ui/design/Theme.h"
#include "ui/layout/Box.h"

namespace ui {

template <typename Content>
void ScrollArea(UiFrameContext &frame, Clay_ElementId id, Clay_Sizing sizing, Content content) {
    Box(frame, {
        .id = id,
        .layout = {
            .sizing = sizing,
            .padding = CLAY_PADDING_ALL(10),
            .childGap = 8,
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = color::SurfaceMuted,
        .cornerRadius = radius::Small,
        .clip = {.vertical = true, .childOffset = Clay_GetScrollOffset()},
        .border = {.color = color::Border, .width = CLAY_BORDER_ALL(1)}
    }, content);
}

}
