#pragma once

#include "ui/design/Colors.h"
#include "ui/design/Radii.h"
#include "ui/design/Spacing.h"
#include "ui/design/Typography.h"

#include <clay.h>

namespace ui {

inline Clay_Sizing grow() {
    return {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)};
}

}
