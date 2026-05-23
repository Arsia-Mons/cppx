#pragma once

#include "ui/design/Theme.h"
#include "ui/layout/Box.h"

namespace ui {

template <typename Content>
void Panel(UiFrameContext &frame, Clay_ElementId id, Clay_Sizing sizing, Content content, uint16_t padding = space::Panel, uint16_t gap = space::Gap) {
    Box(frame, {
        .id = id,
        .layout = {
            .sizing = sizing,
            .padding = CLAY_PADDING_ALL(padding),
            .childGap = gap,
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = color::Surface,
        .cornerRadius = radius::Medium,
        .border = {.color = color::Border, .width = CLAY_BORDER_ALL(1)}
    }, content);
}

}
