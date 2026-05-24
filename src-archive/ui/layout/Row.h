#pragma once

#include "ui/layout/Box.h"

namespace ui {

template <typename Content>
void Row(UiFrameContext &frame, Clay_ElementId id, Clay_Sizing sizing, uint16_t gap, Content content) {
    Box(frame, {
        .id = id,
        .layout = {
            .sizing = sizing,
            .childGap = gap,
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        }
    }, content);
}

}
