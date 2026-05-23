#pragma once

#include "ui/design/Theme.h"
#include "ui/layout/Box.h"
#include "ui/runtime/UiFrameContext.h"

namespace client::ui::screens {

template <typename Content>
void RootShell(::ui::UiFrameContext &frame, Clay_ElementId id, Content content) {
    ::ui::Box(frame, {
        .id = id,
        .layout = {
            .sizing = ::ui::grow(),
            .padding = {.left = ::ui::space::Page, .right = ::ui::space::Page, .top = ::ui::space::Page, .bottom = ::ui::space::Page},
            .childGap = ::ui::space::GapLarge,
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = ::ui::color::Background
    }, content);
}

}
