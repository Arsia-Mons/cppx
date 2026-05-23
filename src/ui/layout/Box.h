#pragma once

#include "ui/runtime/UiFrameContext.h"

#include <clay.h>

namespace ui {

template <typename Content>
void Box(UiFrameContext &, Clay_ElementDeclaration declaration, Content content) {
    CLAY(declaration) {
        content();
    }
}

}
